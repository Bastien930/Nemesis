//
// Created by Basti on 30/12/2025.
//
//
// Dictionary Attack - Optimized version
//

#include "Dictionnary.h"
#include "Config.h"
#include "Utils.h"
#include "log.h"
#include "Hash_Engine.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "brute_force.h"




typedef struct {
    uint_fast64_t line_number;
    uint_fast64_t count;
} dict_resume_t;

static thread_progress_t thread_progress[NEMESIS_MAX_THREADS] = {0};
static dict_resume_t thread_state[NEMESIS_MAX_THREADS];
static int num_display_threads = 0;
static uint_fast64_t total_lines = 0;

// Structure pour le partage du dictionnaire en mémoire
typedef struct {
    char *data;
    size_t size;
    uint_fast64_t *line_offsets;
    uint_fast64_t num_lines;
} dict_mmap_t;

// Affichage de la progression multi-threads
static void print_dict_progress(uint_fast64_t total) {
    printf("\033[%dA", num_display_threads);

    for (int i = 1; i < num_display_threads; i++) {
        printf("\033[K");

        if (thread_progress[i].active) {
            double percent = ((double)thread_progress[i].count * 100.0) / (double)total;
            int bar_width = 80;
            int filled = (int)(percent * bar_width / 100.0);

            printf("T%02d [", i);
            for (int j = 0; j < bar_width; j++) {
                printf("%s", j < filled ? "#" : "-");
            }
            printf("] %5.1f%% | %s\n", percent, thread_progress[i].last_save_word);
        } else {
            printf("T%02d [IDLE]\n", i);
        }
    }

    // Barre globale
    uint_fast64_t sum_current = 0;
    for (int i = 1; i < num_display_threads; i++) {
        sum_current += thread_progress[i].count;
    }

    double global_percent = ((double)sum_current * 100.0) / (total * (num_display_threads - 1));
    int bar_width = 100;
    int filled = (int)(global_percent * bar_width / 100.0);

    printf("\033[K");
    printf("GLOBAL [");
    for (int j = 0; j < bar_width; j++) {
        printf("%s", j < filled ? "-" : "#");
    }
    printf("] %5.1f%%\n", global_percent);

    fflush(stdout);
}

// Compte les lignes du fichier (optimisé)
static uint_fast64_t count_file_lines(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;

    uint_fast64_t lines = 0;
    char buffer[DICT_BUFFER_SIZE];
    size_t bytes;

    while ((bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        for (size_t i = 0; i < bytes; i++) {
            if (buffer[i] == '\n') lines++;
        }
    }

    fclose(f);
    return lines;
}

// Map le fichier en mémoire et indexe les lignes
static int map_dictionary(const char *filename, dict_mmap_t *dict) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        write_log(LOG_ERROR, "Cannot open dictionary file", "dict_attack");
        return -1;
    }

    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        close(fd);
        return -1;
    }

    dict->size = sb.st_size;
    dict->data = mmap(NULL, dict->size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (dict->data == MAP_FAILED) {
        write_log(LOG_ERROR, "mmap failed", "dict_attack");
        return -1;
    }

    // Conseiller au kernel la stratégie de lecture
    madvise(dict->data, dict->size, MADV_SEQUENTIAL);

    // Compter et indexer les lignes
    dict->num_lines = 0;
    for (size_t i = 0; i < dict->size; i++) {
        if (dict->data[i] == '\n') dict->num_lines++;
    }

    // Allouer le tableau d'offsets
    dict->line_offsets = malloc((dict->num_lines + 1) * sizeof(uint_fast64_t));
    if (!dict->line_offsets) {
        munmap(dict->data, dict->size);
        return -1;
    }

    // Remplir les offsets
    uint_fast64_t line_idx = 0;
    dict->line_offsets[line_idx++] = 0;

    for (size_t i = 0; i < dict->size; i++) {
        if (dict->data[i] == '\n') {
            dict->line_offsets[line_idx++] = i + 1;
        }
    }

    return 0;
}

// Extrait une ligne depuis le mmap
static inline void get_line_from_mmap(const dict_mmap_t *dict, uint_fast64_t line_num,
                                      char *buffer, size_t buf_size) {
    if (line_num >= dict->num_lines) {
        buffer[0] = '\0';
        return;
    }

    uint_fast64_t start = dict->line_offsets[line_num];
    uint_fast64_t end = (line_num + 1 < dict->num_lines)
                        ? dict->line_offsets[line_num + 1] - 1
                        : dict->size;

    size_t len = end - start;
    if (len >= buf_size) len = buf_size - 1;

    memcpy(buffer, dict->data + start, len);

    // Retirer le \n ou \r\n
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        len--;
    }

    buffer[len] = '\0';
}

// Sauvegarde de l'état
void save_dict_state(int tid, const dict_resume_t *state) {
    char filename[NEMESIS_MAX_PATH];
    snprintf(filename, sizeof(filename), "resume_dict_thread_%d.bin", tid);
    PATH_JOIN(filename,NEMESIS_MAX_PATH,NEMESIS_config.output.save_dir,filename);
    FILE *f = fopen(filename, "wb");
    if (!f) return;

    fwrite(state, sizeof(dict_resume_t), 1, f);
    fclose(f);
}

// Chargement de l'état
int load_dict_state(int tid, dict_resume_t *state) {
    char filename[NEMESIS_MAX_PATH];
    snprintf(filename, sizeof(filename), "resume_dict_thread_%d.bin", tid);
    PATH_JOIN(filename,NEMESIS_MAX_PATH,NEMESIS_config.output.save_dir,filename);
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;

    unsigned long r = fread(state, sizeof(dict_resume_t), 1, f);
    fclose(f);
    return r >= 1;
}

// Fonction principale d'attaque par dictionnaire
NEMESIS_brute_status_t NEMESIS_dictionary_attack(const char *dict_filename) {
    print_slow("Dictionary attack starting...\n",SPEED_PRINT);

    dict_mmap_t dict = {0};

    if (map_dictionary(dict_filename, &dict) < 0) {
        write_log(LOG_ERROR, "Failed to map dictionary", "dict_attack");
        return NEMESIS_BRUTE_ERROR;
    }

    total_lines = dict.num_lines;
    PRINT_SLOW_MACRO(SPEED_PRINT,"Dictionnaire chargé : %llu lignes\n\n", (unsigned long long)total_lines);

    num_display_threads = NEMESIS_config.system.threads;

    // Initialisation de l'affichage
    for (int i = 1; i < num_display_threads; i++) {
        thread_progress[i].count = 0;
        thread_progress[i].active = 0;
        strcpy(thread_progress[i].last_save_word, "");
        thread_state[i].line_number = 0;
        thread_state[i].count = 0;
        printf("T%02d [Initialisation...]\n", i);
    }

    volatile int stop_ui = 0;

    #pragma omp parallel num_threads(num_display_threads)
    {
        int tid = omp_get_thread_num();

        if (tid == 0) {
            // Thread d'affichage
            sleep(1);
            while (!stop_ui && !interrupt_requested) {
                print_dict_progress(total_lines);

                struct timespec ts = {0, 500 * 1000000};
                while (nanosleep(&ts, &ts) == -1) {
                    if (interrupt_requested) break;
                }
            }
        } else {
            // Threads de travail
            char word[NEMESIS_MAX_LEN + 1];
            uint_fast64_t start_line = 0;
            uint_fast64_t local_count = 0;

            // Reprise si sauvegarde existe
            dict_resume_t state;
            if (NEMESIS_config.input.save && load_dict_state(tid, &state)) {
                start_line = state.line_number;
                local_count = state.count;
                thread_progress[tid].count = local_count;
            }

            // Calcul de la plage de travail pour ce thread
            uint_fast64_t lines_per_thread = total_lines / (num_display_threads - 1);
            uint_fast64_t my_start = start_line + (tid - 1) * lines_per_thread;
            uint_fast64_t my_end = (tid == num_display_threads - 1)
                                   ? total_lines
                                   : my_start + lines_per_thread;

            thread_progress[tid].active = 1;
            #pragma omp flush(thread_progress)

            // Traitement des lignes
            uint_fast64_t next_update = CHUNK_SIZE;

            for (uint_fast64_t line = my_start; line < my_end && !interrupt_requested; line++) {
                if (is_password_found()) break;

                get_line_from_mmap(&dict, line, word, sizeof(word));

                // Ignorer les lignes vides ou trop longues
                if (word[0] == '\0') continue;

                NEMESIS_hash_compare(word, NULL);

                local_count++;

                // Mise à jour périodique par seuil
                if (local_count >= next_update) {
                    thread_progress[tid].count = local_count;
                    strncpy(thread_progress[tid].last_save_word, word, NEMESIS_MAX_LEN);
                    thread_progress[tid].last_save_word[NEMESIS_MAX_LEN] = '\0';

                    thread_state[tid].line_number = line;
                    thread_state[tid].count = local_count;

                    next_update += CHUNK_SIZE;
                }
            }

            #pragma omp critical
            {
                stop_ui = 1;
            }

            thread_progress[tid].active = 0;
        }
    }

    // Nettoyage
    munmap(dict.data, dict.size);
    free(dict.line_offsets);

    end_time();

    if (interrupt_requested) {
        char res[64];
        print_slow("Voulez vous sauvegarder l'etat de l'application ? (Y/N) : ",SPEED_PRINT);
        fflush(stdout);

        if (fgets(res, sizeof(res), stdin) == NULL) return NEMESIS_BRUTE_ERROR;

        if (res[0] != 'Y' && res[0] != 'y') {
            print_slow("Sauvegarde annulée.\n",SPEED_PRINT);

            return NEMESIS_BRUTE_DONE;
        }
        return NEMESIS_BRUTE_INTERRUPTED;
    }

    printf("\n");
    return NEMESIS_BRUTE_DONE;
}

NEMESIS_brute_status_t NEMESIS_dictionary_attack_mangling(const char *dict_filename,ManglingConfig config) {
    print_slow("Debut de l'attaque par dictionnaire...\n",SPEED_PRINT);

    dict_mmap_t dict = {0};

    if (map_dictionary(dict_filename, &dict) < 0) {
        write_log(LOG_ERROR, "Erreur imposible de map le dictionnaire", "NEMESIS_dictionary_attack_mangling");
        return NEMESIS_BRUTE_ERROR;
    }

    total_lines = dict.num_lines;
    int mangling_count = NEMESIS_GET_ITERATION_OF_MANGLING(NEMESIS_config.attack.mangling_config);
    PRINT_SLOW_MACRO(SPEED_PRINT,"Dictionnaire chargé: %llu lignes\n\n", (unsigned long long)total_lines);
    total_lines *= mangling_count;

    num_display_threads = NEMESIS_config.system.threads;

    // Initialisation de l'affichage
    for (int i = 1; i < num_display_threads; i++) {
        thread_progress[i].count = 0;
        thread_progress[i].active = 0;
        strcpy(thread_progress[i].last_save_word, "");
        thread_state[i].line_number = 0;
        thread_state[i].count = 0;
        printf("T%02d [Initialisation...]\n", i);
    }

    volatile int stop_ui = 0;

    #pragma omp parallel num_threads(num_display_threads)
    {
        int tid = omp_get_thread_num();

        if (tid == 0) {
            // Thread d'affichage
            sleep(1);
            while (!stop_ui && !interrupt_requested) {
                print_dict_progress(total_lines);

                struct timespec ts = {0, 500 * 1000000};
                while (nanosleep(&ts, &ts) == -1) {
                    if (interrupt_requested) break;
                }
            }
        } else {
            // Threads de travail
            char word[NEMESIS_MAX_LEN + 1];
            uint_fast64_t start_line = 0;
            uint_fast64_t local_count = 0;

            // Reprise si sauvegarde existe
            dict_resume_t state;
            if (NEMESIS_config.input.save && load_dict_state(tid, &state)) {
                start_line = state.line_number;
                local_count = state.count;
                thread_progress[tid].count = local_count;
            }

            // Calcul de la plage de travail pour ce thread
            uint_fast64_t lines_per_thread = total_lines / (num_display_threads - 1);
            uint_fast64_t my_start = start_line + (tid - 1) * lines_per_thread;
            uint_fast64_t my_end = (tid == num_display_threads - 1)
                                   ? total_lines
                                   : my_start + lines_per_thread;

            thread_progress[tid].active = 1;
            #pragma omp flush(thread_progress)

            // Traitement des lignes
            uint_fast64_t next_update = CHUNK_SIZE_MANGLING;

            for (uint_fast64_t line = my_start; line < my_end && !interrupt_requested; line++) {
                if (is_password_found()) break;

                get_line_from_mmap(&dict, line, word, sizeof(word));

                // Ignorer les lignes vides ou trop longues
                if (word[0] == '\0' ) continue;

                generate_mangled_words(word,&config);

                local_count+=mangling_count;

                // Mise à jour périodique par seuil
                if (local_count >= next_update) {
                    thread_progress[tid].count = local_count;
                    strncpy(thread_progress[tid].last_save_word, word, NEMESIS_MAX_LEN);
                    thread_progress[tid].last_save_word[NEMESIS_MAX_LEN] = '\0';

                    thread_state[tid].line_number = line;
                    thread_state[tid].count = local_count;

                    next_update += CHUNK_SIZE_MANGLING;
                }
            }

            #pragma omp critical
            {
                stop_ui = 1;
            }

            thread_progress[tid].active = 0;
        }
    }

    // Nettoyage
    munmap(dict.data, dict.size);
    free(dict.line_offsets);

    end_time();

    if (interrupt_requested) {
        char res[64];
        print_slow("Voulez vous sauvegarder l'etat de l'application ? (Y/N) : ",SPEED_PRINT);
        fflush(stdout);

        if (fgets(res, sizeof(res), stdin) == NULL) return NEMESIS_BRUTE_ERROR;

        if (res[0] != 'Y' && res[0] != 'y') {
            print_slow("Sauvegarde annulée.\n",SPEED_PRINT);

            return NEMESIS_BRUTE_DONE;
        }
        return NEMESIS_BRUTE_INTERRUPTED;
    }

    printf("\n");
    return NEMESIS_BRUTE_DONE;
}


// Sauvegarde tous les états des threads
void save_dict_thread_states(void) {
    for (int i = 1; i < num_display_threads; i++) {
        if (thread_state[i].count == 0) continue;
        save_dict_state(i, &thread_state[i]);
    }
}

void delete_dict_thread_states(void) {
    for (int i=0;i<NEMESIS_MAX_THREADS;i++) {
        char filename[NEMESIS_MAX_PATH];
        snprintf(filename, sizeof(filename),"resume_dict_thread_%d.bin", i);
        PATH_JOIN(filename,NEMESIS_MAX_PATH,NEMESIS_config.output.save_dir,filename);
        remove(filename);
    }
}