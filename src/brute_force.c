//
// Created by bastien on 10/24/25.
//

#include "brute_force.h"
#include "Config.h"
#include "Utils.h"
#include "log.h"
#include "Hash_Engine.h"
#include "Mangling.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>



static inline void construire_mot_depuis_index(long long n,char *mot,const char *charset,int b, int max_len);
static inline void increment_word(char *word,int *indexes,const char *charset,int b,int *length,int max_len) ;

uint_fast64_t total;


typedef struct {
    int length;
    int indexes[NEMESIS_MAX_LEN];
    uint_fast64_t count;
} brute_resume_t;

static thread_progress_t thread_progress[NEMESIS_MAX_THREADS] = {0};
static int num_display_threads = 0;
brute_resume_t thread_state[NEMESIS_MAX_THREADS];


static void print_multi_progress(uint_fast64_t total_keyspace, uint_fast64_t quota_per_worker) {
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
 * Charger l'état sauvegardé d'un thread
 * @return 1 si OK, 0 sinon
 */
int load_state_from_file(int tid, brute_resume_t *state)
{
    char filename[NEMESIS_MAX_PATH];
    char fullPath[NEMESIS_MAX_PATH];

    snprintf(filename, sizeof(filename), "resume_thread_%d.bin", tid);
    PATH_JOIN(fullPath, NEMESIS_MAX_PATH, NEMESIS_config.output.save_dir, filename);

    FILE *f = fopen(fullPath, "rb");
    if (!f) return 0;

    size_t r = fread(state, sizeof(brute_resume_t), 1, f);
    fclose(f);

    return (r == 1) ? 1 : 0;
}

static inline uint_fast64_t keyspace_total(int charset_len, int min_len, int max_len) {
    uint_fast64_t tot = 0;
    for (int L = min_len; L <= max_len; L++) {
        tot += puissance((uint_fast64_t)charset_len, L);
    }
    return tot;
}

/**
 * Exécuter une attaque par force brute
 */
NEMESIS_brute_status_t NEMESIS_bruteforce(void) {

    print_slow("Debut de l'attaque Bruteforce...\n", SPEED_PRINT);

    char caracteres[256];

    int len = build_charset(
        caracteres,
        sizeof(caracteres),
        NEMESIS_config.attack.charset_preset,
        NEMESIS_config.attack.charset_custom
    );

    const int min_len = NEMESIS_config.attack.min_len;
    const int max_len = NEMESIS_config.attack.max_len;

    if (len < 0) {
        write_log(LOG_ERROR, "Erreur charset", "bruteforce");
        return NEMESIS_BRUTE_ERROR;
    }

    const int b = len;

    num_display_threads = NEMESIS_config.system.threads;
    if (num_display_threads < 2) {
        write_log(LOG_ERROR, "threads doit etre >= 2 (1 UI + 1 worker)", "bruteforce");
        return NEMESIS_BRUTE_ERROR;
    }

    const int workers = num_display_threads - 1;

    const uint_fast64_t total_keyspace = keyspace_total(len, min_len, max_len);
    const uint_fast64_t quota_per_worker = (total_keyspace + (uint_fast64_t)workers - 1u) / (uint_fast64_t)workers; // division plafond


    // Préparer l'affichage (workers + GLOBAL)
    for (int tid = 1; tid <= workers; tid++) {
        #pragma omp atomic write
        thread_progress[tid].count = 0;

        #pragma omp atomic write
        thread_progress[tid].active = 0;

        #pragma omp critical(progress_string)
        {
            thread_progress[tid].last_save_word[0] = '\0';
        }

        thread_state[tid].length = 0;
        memset(thread_state[tid].indexes, 0, sizeof(int) * max_len);
        printf("T%02d [Initialisation...]\n", tid);
    }
    printf("GLOBAL [Initialisation...]\n");

    int stop_ui = 0;

    #pragma omp parallel num_threads(num_display_threads)
    {
        int tid = omp_get_thread_num();

        if (tid == 0) {
            // UI thread
            sleep(1);

            while (1) {
                int s = 0;
                #pragma omp atomic read
                s = stop_ui;

                if (s || interrupt_requested) break;

                print_multi_progress(total_keyspace, quota_per_worker);

                struct timespec ts = {0, 250 * 1000000}; // 250ms
                while (nanosleep(&ts, &ts) == -1) {
                    if (interrupt_requested) break;
                }
            }
        } else {
            // Worker
            const int worker_index = tid - 1; // 0..workers-1

            char word[max_len + 1];
            int indexes[max_len];
            int length = min_len;

            // Init indexes/word
            memset(indexes, 0, sizeof(indexes));
            for (int i = 0; i < min_len; i++) word[i] = caracteres[0];
            word[min_len] = '\0';

            // Resume si dispo
            brute_resume_t state;
            int resumed = 0;
            if (NEMESIS_config.input.save && load_state_from_file(tid, &state)) {
                length = state.length;
                memcpy(indexes, state.indexes, sizeof(int) * max_len);

                for (int i = 0; i < length; i++) word[i] = caracteres[indexes[i]];
                word[length] = '\0';

                #pragma omp atomic write
                thread_progress[tid].count = state.count;

                thread_state[tid].count = state.count;
                thread_state[tid].length = length;
                memcpy(thread_state[tid].indexes, indexes, sizeof(int) * max_len);

                #pragma omp critical(progress_string)
                {
                    strncpy(thread_progress[tid].last_save_word, word, max_len);
                    thread_progress[tid].last_save_word[max_len] = '\0';
                }

                resumed = 1;
            }

            if (!resumed) {
                // Offset initial : worker_index pas
                for (int k = 0; k < worker_index; k++) {
                    increment_word(word, indexes, caracteres, b, &length, max_len);
                    if (length > max_len) break;
                }
            }

            #pragma omp atomic write
            thread_progress[tid].active = 1;

            const long long UPDATE_INTERVAL = (long long)CHUNK_SIZE;
            long long local_counter = 0;
            uint_fast64_t done = 0;

            while (!interrupt_requested && !is_password_found() && done < quota_per_worker) {

                // Tester
                NEMESIS_hash_compare(word, NULL);

                // Avancer de "workers" pas (partition stable)
                for (int k = 0; k < workers; k++) {
                    increment_word(word, indexes, caracteres, b, &length, max_len);
                    if (length > max_len) break;
                }
                if (length > max_len) break;

                done++;
                #pragma omp atomic update
                thread_progress[tid].count += 1;

                local_counter++;

                if (local_counter >= UPDATE_INTERVAL) {
                #pragma omp critical(progress_string)
                    {
                        strncpy(thread_progress[tid].last_save_word, word, max_len);
                        thread_progress[tid].last_save_word[max_len] = '\0';
                    }

                    thread_state[tid].length = length;
                    uint_fast64_t c = 0;
                    #pragma omp atomic read
                    c = thread_progress[tid].count;
                    thread_state[tid].count = c;
                    memcpy(thread_state[tid].indexes, indexes, sizeof(int) * max_len);

                    local_counter = 0;
                }
            }

            if (local_counter > 0) {
#pragma omp critical(progress_string)
                {
                    strncpy(thread_progress[tid].last_save_word, word, max_len);
                    thread_progress[tid].last_save_word[max_len] = '\0';
                }
                thread_state[tid].length = length;

                // ← manque ça :
                uint_fast64_t c = 0;
#pragma omp atomic read
                c = thread_progress[tid].count;
                thread_state[tid].count = c;

                memcpy(thread_state[tid].indexes, indexes, sizeof(int) * max_len);
            }

            // Signaler à l'UI d'arrêter (un seul worker suffit)
            #pragma omp atomic write
            stop_ui = 1;
        }
    }

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
 * Exécuter une attaque par force brute avec mangling
 *
 * @param config Configuration des règles de mangling à appliquer
 * @return Status de l'attaque (DONE, ERROR, INTERRUPTED)
 */
/*NEMESIS_brute_status_t NEMESIS_bruteforce_mangling(ManglingConfig config) {

    print_slow("Debut de l'attaque Bruteforce...\n",SPEED_PRINT);

    char caracteres[256];


    int len = build_charset(
        caracteres,
        sizeof(caracteres),
        NEMESIS_config.attack.charset_preset,
        NEMESIS_config.attack.charset_custom
    );

    if (len < 0) {
        write_log(LOG_ERROR, "Erreur charset", "bruteforce");
        return NEMESIS_BRUTE_ERROR;
    }

    const int b = len;


    num_display_threads = NEMESIS_config.system.threads;
    int mangling_count = NEMESIS_GET_ITERATION_OF_MANGLING(NEMESIS_config.attack.mangling_config);
    total = puissance(len, NEMESIS_MAX_LEN)*mangling_count/num_display_threads;



    for (int i = 1; i < num_display_threads; i++) {
        thread_progress[i].count = 0;
        thread_progress[i].active = 0;
        strcpy(thread_progress[i].last_save_word, "");
        thread_state[i].length = 0;
        memset(thread_state[i].indexes, 0, sizeof(int) * NEMESIS_MAX_LEN);
        printf("T%02d [Initialisation...]\n", i);
    }

    volatile int stop_ui = 0;

    #pragma omp parallel num_threads(num_display_threads)
    {
        int tid = omp_get_thread_num();



        if (tid==0) {
            sleep(3);
            while (!stop_ui && !interrupt_requested) {

                print_multi_progress(LL(total));

                struct timespec ts = {0, 500 * 1000000};
                while (nanosleep(&ts, &ts) == -1) {
                    if (interrupt_requested)
                        break;
                }
            }
        } else {
            int flag = 0;
            char word[NEMESIS_MAX_LEN + 1];
            int indexes[NEMESIS_MAX_LEN];
            int length = 1;

            brute_resume_t state;
            if (NEMESIS_config.input.save && load_state_from_file(tid, &state)) {
                length = state.length;
                memcpy(indexes, state.indexes, sizeof(int) * NEMESIS_MAX_LEN);
                for (int i = 0; i < length; i++)
                    word[i] = caracteres[indexes[i]];
                word[length] = '\0';
                strcpy(thread_progress[tid].last_save_word,word);
                thread_state[tid].count = state.count;
                thread_progress[tid].count = state.count;
            } else {


                // mot initial : "a"
                indexes[0] = 0;
                word[0] = caracteres[0];
                word[1] = '\0';

                // DÉCALAGE INITIAL
                for (int i = 1; i < tid; i++) {
                    increment_word(word, indexes, caracteres, b, &length, NEMESIS_MAX_LEN);
                }
            }

            thread_progress[tid].active = 1;
#pragma omp flush(thread_progress)

            const long long UPDATE_INTERVAL = CHUNK_SIZE_MANGLING;
            long long local_counter = 0;

            while (!interrupt_requested && !is_password_found()) {


                generate_mangled_words(word,&config);

                // saut de num_threads mots
                for (int i = 1; i < num_display_threads; i++) {
                    increment_word(word, indexes, caracteres, b, &length, NEMESIS_MAX_LEN);
                }

                if (length > NEMESIS_MAX_LEN)
                    break;

                local_counter+=mangling_count;

                if (local_counter >= UPDATE_INTERVAL) {
                    thread_progress[tid].count += local_counter;
                    strncpy(thread_progress[tid].last_save_word,word,NEMESIS_MAX_LEN);
                    thread_progress[tid].last_save_word[NEMESIS_MAX_LEN] = '\0';

                    thread_state[tid].length = length;
                    thread_state[tid].count = thread_progress[tid].count;
                    memcpy(thread_state[tid].indexes, indexes, sizeof(int) * NEMESIS_MAX_LEN);

                    local_counter = 0;
                }
            }
            #pragma omp critical
            {
                stop_ui = 1;
            }
        }
    }


    //Sauvegarde
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
    fflush(stdout);
    return NEMESIS_BRUTE_DONE;
}*/


/**
 * Sauvegarder l'état de progression de chaque thread
 */
void save_brute_thread_states(void) {
    for (int i = 1; i < num_display_threads; i++) {
        if (thread_state[i].count == 0)
            continue;

        char fullPath[NEMESIS_MAX_PATH];
        char filename[NEMESIS_MAX_PATH];
        snprintf(filename, sizeof(filename),"resume_thread_%d.bin", i);
        PATH_JOIN(fullPath,NEMESIS_MAX_PATH,NEMESIS_config.output.save_dir,filename);
        FILE *f = fopen(fullPath, "wb");
        if (!f) {write_log(LOG_ERROR, "Erreur lors de la sauvegarde du fichier de progression", "bruteforce");perror("save");return;}

        fwrite(&thread_state[i], sizeof(brute_resume_t), 1, f);
        fclose(f);
    }
}

/**
 * Supprimer les fichiers de sauvegarde d'état des threads
 */
void delete_brut_thread_states(void) {
    for (int i=0;i<NEMESIS_MAX_THREADS;i++) {
        char filename[NEMESIS_MAX_PATH];
        char fullPath[NEMESIS_MAX_PATH];
        snprintf(filename, sizeof(filename),"resume_thread_%d.bin", i);
        PATH_JOIN(fullPath,NEMESIS_MAX_PATH,NEMESIS_config.output.save_dir,filename);
        remove(fullPath);
    }
}


/**
 * Construire un jeu de caractères selon le preset choisi
 * 
 * @param out Buffer de sortie pour le charset
 * @param out_size Taille du buffer de sortie
 * @param preset Type de charset prédéfini à utiliser
 * @param custom_charset Charset personnalisé si preset est CUSTOM
 * @return Nombre de caractères dans le charset ou erreur si négatif
 */
int build_charset(char *out, size_t out_size,
                  NEMESIS_charset_preset_t preset,
                  const char *custom_charset)
{
    size_t pos = 0;

    switch (preset) {

        case NEMESIS_CHARSET_PRESET_DEFAULT:
            // ASCII imprimable (33 à 126) pas l'espace
            for (int c = 33; c <= 126; c++) {
                if (pos + 1 >= out_size) return -1;
                out[pos++] = (char)c;
            }
            break;

        case NEMESIS_CHARSET_PRESET_ALPHANUM:
            // A-Z
            for (char c = 'A'; c <= 'Z'; c++) out[pos++] = c;
            // a-z
            for (char c = 'a'; c <= 'z'; c++) out[pos++] = c;
            // 0-9
            for (char c = '0'; c <= '9'; c++) out[pos++] = c;
            break;

        case NEMESIS_CHARSET_PRESET_NUMERIC:
            for (char c = '0'; c <= '9'; c++) out[pos++] = c;
            break;

        case NEMESIS_CHARSET_PRESET_CUSTOM:
            if (!custom_charset) return -2;
            pos = strlen(custom_charset);
            if (pos + 1 >= out_size) return -3;
            memcpy(out, custom_charset, pos);
            break;

        default:
            return -4;
    }

    out[pos] = '\0';
    return pos;       // retourne la taille
}

static inline void increment_word(char *word,int *indexes,const char *charset,int b,int *length,int max_len) {
    int pos = 0;

    while (pos < *length) {
        indexes[pos]++;
        if (indexes[pos] < b) {
            word[pos] = charset[indexes[pos]];
            return;
        }
        indexes[pos] = 0;
        word[pos] = charset[0];
        pos++;
    }

    if (*length < max_len) {
        indexes[*length] = 0;
        word[*length] = charset[0];
        (*length)++;
        word[*length] = '\0';
    }
}