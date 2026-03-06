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
#include "brute_force.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>





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
/**
 * Afficher la progression de l'attaque par dictionnaire pour tous les threads
 *
 * @param total Nombre total de lignes à traiter
 */
static void print_dict_progress(uint_fast64_t total_keyspace, uint_fast64_t quota_per_worker) {
    const int workers = num_display_threads - 1;
    const int lines_to_redraw = workers + 1; // workers + ligne GLOBAL

    // Remonter de N lignes
    printf("\033[%dA", lines_to_redraw);

    // Barres par thread (workers uniquement)
    for (int tid = 1; tid <= workers; tid++) {
        uint_fast64_t c = 0;
        int active = 0;
        char last[512]; // fallback local (si last_save_word plus grand, adapte)
        last[0] = '\0';

        #pragma omp atomic read
        c = thread_progress[tid].count;

        #pragma omp atomic read
        active = thread_progress[tid].active;

        #pragma omp critical(progress_string)
        {
            // copie défensive
            strncpy(last, thread_progress[tid].last_save_word, sizeof(last) - 1);
            last[sizeof(last) - 1] = '\0';
        }

        printf("\033[K"); // clear line

        if (active) {
            double percent = 0.0;
            if (quota_per_worker > 0) {
                percent = ((double)c * 100.0) / (double)quota_per_worker;
            }

            int bar_width = 80;
            int filled = (int)(percent * (double)bar_width / 100.0);
            if (filled < 0) filled = 0;
            if (filled > bar_width) filled = bar_width;

            printf("T%02d [", tid);
            for (int j = 0; j < bar_width; j++) {
                putchar(j < filled ? '#' : '-');
            }
            printf("] %5.1f%% | %s\n", percent, last);
        } else {
            printf("T%02d [IDLE]\n", tid);
        }
    }

    // Barre globale
    uint_fast64_t sum_current = 0;
    for (int tid = 1; tid <= workers; tid++) {
        uint_fast64_t c = 0;
        #pragma omp atomic read
        c = thread_progress[tid].count;
        sum_current += c;
    }

    double global_percent = 0.0;
    if (total_keyspace > 0) {
        global_percent = ((double)sum_current * 100.0) / (double)total_keyspace;
    }

    int bar_width = 100;
    int filled = (int)(global_percent * (double)bar_width / 100.0);
    if (filled < 0) filled = 0;
    if (filled > bar_width) filled = bar_width;

    printf("\033[K");
    printf("GLOBAL [");
    for (int j = 0; j < bar_width; j++) {
        putchar(j < filled ? '#' : '-');
    }
    printf("] %5.1f%%\n", global_percent);

    fflush(stdout);
}

/**
 * Compter le nombre de lignes dans un fichier
 *
 * @param filename Chemin du fichier à analyser
 * @return Nombre de lignes dans le fichier, 0 si erreur
 */
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

/**
 * Charger un dictionnaire en mémoire avec mmap et indexer ses lignes
 *
 * @param filename Chemin du fichier dictionnaire
 * @param dict Structure à remplir avec les données mappées
 * @return 0 en cas de succès, -1 si erreur
 */
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

    // pour le kernel
    madvise(dict->data, dict->size, MADV_SEQUENTIAL);

    // Compter et indexer les lignes
    dict->num_lines = 0;
    for (size_t i = 0; i < dict->size; i++) {
        if (dict->data[i] == '\n') dict->num_lines++;
    }

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

/**
 * Extraire une ligne spécifique du dictionnaire mappé en mémoire
 *
 * @param dict Structure contenant les données mappées
 * @param line_num Numéro de la ligne à extraire
 * @param buffer Buffer de destination
 * @param buf_size Taille du buffer
 */
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

/**
 * Sauvegarder l'état d'avancement d'un thread
 *
 * @param tid Identifiant du thread
 * @param state État à sauvegarder
 */
void save_dict_state(int tid, const dict_resume_t *state) {
    char filename[NEMESIS_MAX_PATH];
    char fullPath[NEMESIS_MAX_PATH];
    snprintf(filename, sizeof(filename), "resume_dict_thread_%d.bin", tid);
    PATH_JOIN(fullPath,NEMESIS_MAX_PATH,NEMESIS_config.output.save_dir,filename);
    FILE *f = fopen(fullPath, "wb");
    if (!f) return;

    fwrite(state, sizeof(dict_resume_t), 1, f);
    fclose(f);
}

/**
 * Charger l'état d'avancement sauvegardé d'un thread
 *
 * @param tid Identifiant du thread
 * @param state Structure à remplir avec l'état chargé
 * @return -1 si erreur, nombre d'états chargés sinon
 */
int load_dict_state(int tid, dict_resume_t *state) {
    char filename[NEMESIS_MAX_PATH];
    char fullPath[NEMESIS_MAX_PATH];
    snprintf(filename, sizeof(filename), "resume_dict_thread_%d.bin", tid);
    PATH_JOIN(fullPath,NEMESIS_MAX_PATH,NEMESIS_config.output.save_dir,filename);
    FILE *f = fopen(fullPath, "rb");
    if (!f) return -1;

    unsigned long r = fread(state, sizeof(dict_resume_t), 1, f);
    fclose(f);
    return r >= 1;
}

/**
 * Lancer une attaque par dictionnaire
 *
 * @param dict_filename Chemin du fichier dictionnaire
 * @return Status de l'attaque
 */
NEMESIS_brute_status_t NEMESIS_dictionary_attack(const char *dict_filename) {
    print_slow("Dictionary attack starting...\n", SPEED_PRINT);

    dict_mmap_t dict = {0};

    if (map_dictionary(dict_filename, &dict) < 0) {
        write_log(LOG_ERROR, "Failed to map dictionary", "dict_attack");
        return NEMESIS_BRUTE_ERROR;
    }

    total_lines = dict.num_lines;
    PRINT_SLOW_MACRO(SPEED_PRINT, "Dictionnaire chargé : %llu lignes\n\n", (unsigned long long)total_lines);

    num_display_threads = NEMESIS_config.system.threads;
    if (num_display_threads < 2) {
        write_log(LOG_ERROR, "threads doit etre >= 2 (1 UI + 1 worker)", "dict_attack");
        return NEMESIS_BRUTE_ERROR;
    }

    const int workers = num_display_threads - 1;
    const uint_fast64_t quota_per_worker = (total_lines + (uint_fast64_t)workers - 1u) / (uint_fast64_t)workers;

    // Initialisation affichage
    for (int i = 1; i <= workers; i++) {
        #pragma omp atomic write
        thread_progress[i].count = 0;

        #pragma omp atomic write
        thread_progress[i].active = 0;

        #pragma omp critical(progress_string)
        {
            thread_progress[i].last_save_word[0] = '\0';
        }

        thread_state[i].line_number = 0;
        thread_state[i].count = 0;
        printf("T%02d [Initialisation...]\n", i);
    }
    printf("GLOBAL [Initialisation...]\n");

    int stop_ui = 0;

    #pragma omp parallel num_threads(num_display_threads)
    {
        int tid = omp_get_thread_num();

        if (tid == 0) {
            // Thread UI
            sleep(1);

            while (1) {
                int s = 0;
                #pragma omp atomic read
                s = stop_ui;

                if (s || interrupt_requested) break;

                print_dict_progress(total_lines, quota_per_worker);

                struct timespec ts = {0, 250 * 1000000};
                while (nanosleep(&ts, &ts) == -1) {
                    if (interrupt_requested) break;
                }
            }
        } else {
            // Worker
            char word[256 + 1];

            // Calcul plage de base
            uint_fast64_t my_start = (uint_fast64_t)(tid - 1) * quota_per_worker;
            uint_fast64_t my_end = my_start + quota_per_worker;
            if (my_end > total_lines) my_end = total_lines;

            // Reprise si sauvegarde
            dict_resume_t state;
            if (NEMESIS_config.input.save && load_dict_state(tid, &state)) {
                if (state.line_number > my_start)
                    my_start = state.line_number;

                #pragma omp atomic write
                thread_progress[tid].count = state.count;
                thread_state[tid].count = state.count;
            }

            #pragma omp atomic write
            thread_progress[tid].active = 1;

            long long local_counter = 0;
            const long long UPDATE_INTERVAL = (long long)CHUNK_SIZE;

            for (uint_fast64_t line = my_start; line < my_end && !interrupt_requested; line++) {
                if (is_password_found()) break;

                get_line_from_mmap(&dict, line, word, sizeof(word));
                if (word[0] == '\0') continue;

                NEMESIS_hash_compare(word, NULL);

                // Update atomique du count à chaque ligne
                #pragma omp atomic update
                thread_progress[tid].count += 1;

                local_counter++;

                // Update string + state au seuil seulement
                if (local_counter >= UPDATE_INTERVAL) {
                    #pragma omp critical(progress_string)
                    {
                        strncpy(thread_progress[tid].last_save_word, word, 255);
                        thread_progress[tid].last_save_word[256] = '\0';
                    }

                    thread_state[tid].line_number = line;
                    uint_fast64_t c = 0;
                    #pragma omp atomic read
                    c = thread_progress[tid].count;
                    thread_state[tid].count = c;

                    local_counter = 0;
                }
            }

            // Flush final
            if (local_counter > 0) {
                #pragma omp critical(progress_string)
                {
                    strncpy(thread_progress[tid].last_save_word, word, 255);
                    thread_progress[tid].last_save_word[256] = '\0';
                }

                thread_state[tid].line_number = my_end - 1;
                uint_fast64_t c = 0;
                #pragma omp atomic read
                c = thread_progress[tid].count;
                thread_state[tid].count = c;
            }

            #pragma omp atomic write
            thread_progress[tid].active = 0;

            #pragma omp atomic write
            stop_ui = 1;
        }
    }

    munmap(dict.data, dict.size);
    free(dict.line_offsets);
    end_time();

    if (interrupt_requested) {
        char res[64];
        print_slow("Voulez vous sauvegarder l'etat de l'application ? (Y/N) : ", SPEED_PRINT);
        fflush(stdout);
        if (fgets(res, sizeof(res), stdin) == NULL) return NEMESIS_BRUTE_ERROR;
        if (res[0] != 'Y' && res[0] != 'y') {
            print_slow("Sauvegarde annulée.\n", SPEED_PRINT);
            return NEMESIS_BRUTE_DONE;
        }
        return NEMESIS_BRUTE_INTERRUPTED;
    }

    printf("\n");
    fflush(stdout);
    return NEMESIS_BRUTE_DONE;
}

/**
 * Lancer une attaque par dictionnaire avec règles de mutation
 *
 * @param dict_filename Chemin du fichier dictionnaire
 * @param config Configuration des règles de mutation
 * @return Status de l'attaque
 */
NEMESIS_brute_status_t NEMESIS_dictionary_attack_mangling(const char *dict_filename, ManglingConfig config) {
    print_slow("Debut de l'attaque par dictionnaire...\n", SPEED_PRINT);

    dict_mmap_t dict = {0};

    if (map_dictionary(dict_filename, &dict) < 0) {
        write_log(LOG_ERROR, "Erreur imposible de map le dictionnaire", "NEMESIS_dictionary_attack_mangling");
        return NEMESIS_BRUTE_ERROR;
    }

    total_lines = dict.num_lines;
    int mangling_count = NEMESIS_GET_ITERATION_OF_MANGLING(NEMESIS_config.attack.mangling_config);
    PRINT_SLOW_MACRO(SPEED_PRINT, "Dictionnaire chargé: %llu lignes\n\n", (unsigned long long)total_lines);
    total_lines *= mangling_count;

    num_display_threads = NEMESIS_config.system.threads;
    if (num_display_threads < 2) {
        write_log(LOG_ERROR, "threads doit etre >= 2 (1 UI + 1 worker)", "NEMESIS_dictionary_attack_mangling");
        return NEMESIS_BRUTE_ERROR;
    }

    const int workers = num_display_threads - 1;
    const uint_fast64_t display_total = total_lines; // déjà multiplié
    const uint_fast64_t real_lines = dict.num_lines;  // lignes réelles du dico
    const uint_fast64_t quota_per_worker = (real_lines + (uint_fast64_t)workers - 1u) / (uint_fast64_t)workers;

    // Pour l'affichage, quota en termes de mots manglés
    const uint_fast64_t display_quota = quota_per_worker * (uint_fast64_t)mangling_count;

    // Initialisation affichage
    for (int i = 1; i <= workers; i++) {
        #pragma omp atomic write
        thread_progress[i].count = 0;

        #pragma omp atomic write
        thread_progress[i].active = 0;

        #pragma omp critical(progress_string)
        {
            thread_progress[i].last_save_word[0] = '\0';
        }

        thread_state[i].line_number = 0;
        thread_state[i].count = 0;
        printf("T%02d [Initialisation...]\n", i);
    }
    printf("GLOBAL [Initialisation...]\n");

    int stop_ui = 0;

    #pragma omp parallel num_threads(num_display_threads)
    {
        int tid = omp_get_thread_num();

        if (tid == 0) {
            // Thread UI
            sleep(1);

            while (1) {
                int s = 0;
                #pragma omp atomic read
                s = stop_ui;

                if (s || interrupt_requested) break;

                print_dict_progress(display_total, display_quota);

                struct timespec ts = {0, 250 * 1000000};
                while (nanosleep(&ts, &ts) == -1) {
                    if (interrupt_requested) break;
                }
            }
        } else {
            // Worker
            char word[256 + 1];
            uint_fast64_t local_count = 0;

            // Calcul plage de base
            uint_fast64_t my_start = (uint_fast64_t)(tid - 1) * quota_per_worker;
            uint_fast64_t my_end = my_start + quota_per_worker;
            if (my_end > real_lines) my_end = real_lines;  // ← real_lines pas total_lines

            // Reprise si sauvegarde
            dict_resume_t state;
            if (NEMESIS_config.input.save && load_dict_state(tid, &state)) {
                if (state.line_number > my_start)
                    my_start = state.line_number;
                local_count = state.count;

                #pragma omp atomic write
                thread_progress[tid].count = local_count;
                thread_state[tid].count = local_count;
            }

            #pragma omp atomic write
            thread_progress[tid].active = 1;

            long long local_counter = 0;
            const long long UPDATE_INTERVAL = (long long)CHUNK_SIZE_MANGLING;
            HashSet *hs = malloc(sizeof(HashSet));
            hashset_init(hs);
            for (uint_fast64_t line = my_start; line < my_end && !interrupt_requested; line++) {
                if (is_password_found()) break;

                get_line_from_mmap(&dict, line, word, sizeof(word));
                if (word[0] == '\0') continue;

                generate_mangled_words(word, &config,hs);

                local_count += mangling_count;
                local_counter += mangling_count;

                thread_progress[tid].count += (uint_fast64_t)mangling_count;

                if (local_counter >= UPDATE_INTERVAL) {
#pragma omp critical(progress_string)
                    {
                        strncpy(thread_progress[tid].last_save_word, word, 255);
                        thread_progress[tid].last_save_word[256] = '\0';
                    }

                    thread_state[tid].line_number = line;
                    uint_fast64_t c = 0;
#pragma omp atomic read
                    c = thread_progress[tid].count;
                    thread_state[tid].count = c;

                    local_counter = 0;
                }
            }
            hashset_free(hs);
            free(hs);
            // Flush final
            if (local_counter > 0) {
                #pragma omp atomic update
                thread_progress[tid].count += (uint_fast64_t)local_counter;

                #pragma omp critical(progress_string)
                {
                    strncpy(thread_progress[tid].last_save_word, word, 255);
                    thread_progress[tid].last_save_word[256] = '\0';
                }

                thread_state[tid].line_number = my_end - 1;
                uint_fast64_t c = 0;
                #pragma omp atomic read
                c = thread_progress[tid].count;
                thread_state[tid].count = c;
            }


            #pragma omp atomic write
            thread_progress[tid].active = 0;

            // Signaler à l'UI
            #pragma omp atomic write
            stop_ui = 1;
        }
    }

    munmap(dict.data, dict.size);
    free(dict.line_offsets);
    end_time();

    if (interrupt_requested) {
        char res[64];
        print_slow("Voulez vous sauvegarder l'etat de l'application ? (Y/N) : ", SPEED_PRINT);
        fflush(stdout);
        if (fgets(res, sizeof(res), stdin) == NULL) return NEMESIS_BRUTE_ERROR;
        if (res[0] != 'Y' && res[0] != 'y') {
            print_slow("Sauvegarde annulée.\n", SPEED_PRINT);
            return NEMESIS_BRUTE_DONE;
        }
        return NEMESIS_BRUTE_INTERRUPTED;
    }

    printf("\n");
    fflush(stdout);
    return NEMESIS_BRUTE_DONE;
}


/**
 * Sauvegarder l'état de tous les threads actifs
 */
void save_dict_thread_states(void) {
    for (int i = 1; i < num_display_threads; i++) {
        if (thread_state[i].count == 0) continue;
        save_dict_state(i, &thread_state[i]);
    }
}

/**
 * Supprimer tous les fichiers de sauvegarde d'état des threads
 */
void delete_dict_thread_states(void) {
    for (int i=0;i<NEMESIS_MAX_THREADS;i++) {
        char filename[NEMESIS_MAX_PATH];
        char fullPath[NEMESIS_MAX_PATH];
        snprintf(filename, sizeof(filename),"resume_dict_thread_%d.bin", i);
        PATH_JOIN(fullPath,NEMESIS_MAX_PATH,NEMESIS_config.output.save_dir,filename);
        remove(fullPath);
    }
}
