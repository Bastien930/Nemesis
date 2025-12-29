//
// Created by bastien on 10/24/25.
//

#include "brute_force.h"
#include "Config.h"
#include "Utils.h"
#include "log.h"


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../Include/brute_force.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>

#include "Hash_Engine.h"
#include "main.h"
#include "Mangling.h"

static inline void construire_mot_depuis_index(long long n,char *mot,const char *charset,int b, int max_len);
static inline void increment_word(char *word,int *indexes,const char *charset,int b,int *length,int max_len) ;

uint_fast64_t total;

typedef struct {
    uint_fast64_t count;
    char last_save_word[DA_MAX_LEN + 1];
    int active;
} thread_progress_t ; // line de cache unique.

typedef struct {
    int length;
    int indexes[DA_MAX_LEN];
    uint_fast64_t count;
} brute_resume_t;

static thread_progress_t thread_progress[DA_MAX_THREADS] = {0};
static int num_display_threads = 0;
brute_resume_t thread_state[DA_MAX_THREADS];

// Fonction pour afficher toutes les barres, plus une barre globale
static void print_multi_progress(uint_fast64_t total) {
    // Remonter de N lignes (où N = nombre de threads)
    printf("\033[%dA", num_display_threads );

    // Barres par thread
    for (int i = 1; i < num_display_threads; i++) {
        printf("\033[K");  // Effacer la ligne

        if (thread_progress[i].active) {
            double percent = ((double)thread_progress[i].count * 100.0) / (double)total;
            int bar_width = 80;
            int filled = (int)(percent * bar_width / 100.0);

            printf("T%02d [", i);
            for (int j = 0; j < bar_width; j++) {
                printf("%s", j < filled ? "-" : "#");
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

    double global_percent = ((double)sum_current * 100.0) / (total * (num_display_threads-1));
    int bar_width = 100;  // largeur différente pour la barre globale
    int filled = (int)(global_percent * bar_width / 100.0);

    printf("\033[K");  // Effacer la ligne
    printf("GLOBAL [");
    for (int j = 0; j < bar_width; j++) {
        printf("%s", j < filled ? "-" : "#");
    }
    printf("] %5.1f%%\n", global_percent);

    fflush(stdout);
}

void save_state_to_file(int tid, const brute_resume_t *state)
{
    char filename[64];
    snprintf(filename, sizeof(filename), "resume_thread_%d.bin", tid);

    FILE *f = fopen(filename, "wb");
    if (!f) return;

    fwrite(state, sizeof(brute_resume_t), 1, f);
    fclose(f);
}


int load_state_from_file(int tid, brute_resume_t *state)
{
    char filename[64];
    snprintf(filename, sizeof(filename), "resume_thread_%d.bin", tid);

    FILE *f = fopen(filename, "rb");
    if (!f){return 0;}

    int r = fread(state, sizeof(brute_resume_t), 1, f);
    fclose(f);
    return 1;
}


da_brute_status_t da_bruteforce(void) {

    printf("dans brute force\n");

    char caracteres[256];


    int len = build_charset(
        caracteres,
        sizeof(caracteres),
        da_config.attack.charset_preset,
        da_config.attack.charset_custom
    );

    if (len < 0) {
        write_log(LOG_ERROR, "Erreur charset", "bruteforce");
        return DA_BRUTE_ERROR;
    }

    const int b = len;


    num_display_threads = da_config.system.threads;
    total = puissance(len, DA_MAX_LEN)/num_display_threads;



    for (int i = 1; i < num_display_threads; i++) {
        thread_progress[i].count = 0;
        thread_progress[i].active = 0;
        strcpy(thread_progress[i].last_save_word, "");
        thread_state[i].length = 0;
        memset(thread_state[i].indexes, 0, sizeof(int) * DA_MAX_LEN);
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
            char word[DA_MAX_LEN + 1];
            int indexes[DA_MAX_LEN];
            int length = 1;

            brute_resume_t state;
            if (da_config.input.save && load_state_from_file(tid, &state)) {
                length = state.length;
                memcpy(indexes, state.indexes, sizeof(int) * DA_MAX_LEN);
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

                // =========================
                // DÉCALAGE INITIAL (IMPORTANT)
                // =========================
                for (int i = 1; i < tid; i++) {
                    increment_word(word, indexes, caracteres, b, &length, DA_MAX_LEN);
                }
            }

            thread_progress[tid].active = 1;
#pragma omp flush(thread_progress)

            const long long UPDATE_INTERVAL = CHUNK_SIZE;
            long long local_counter = 0;

            while (!interrupt_requested && !is_password_found()) {


                da_hash_compare(word, NULL);

                // saut de num_threads mots
                for (int i = 1; i < num_display_threads; i++) {
                    increment_word(word, indexes, caracteres, b, &length, DA_MAX_LEN);
                }

                if (length > DA_MAX_LEN)
                    break;

                local_counter++;

                if (local_counter >= UPDATE_INTERVAL) {
                    thread_progress[tid].count += local_counter;
                    strncpy(thread_progress[tid].last_save_word,word,DA_MAX_LEN);
                    thread_progress[tid].last_save_word[DA_MAX_LEN] = '\0';

                    thread_state[tid].length = length;
                    thread_state[tid].count = thread_progress[tid].count;
                    memcpy(thread_state[tid].indexes, indexes, sizeof(int) * DA_MAX_LEN);

                    local_counter = 0;
                }
            }
            #pragma omp critical
            {
                stop_ui = 1;
            }
        }
    }


    // =========================
    // SAUVEGARDE ET FIN
    // =========================

    end_time();
    if (interrupt_requested) {
        char res[64];
        printf("Voulez vous sauvegarder l'etat de l'application ? (Y/N) : ");
        fflush(stdout);

        if (fgets(res, sizeof(res), stdin) == NULL) return DA_BRUTE_ERROR;

        if (res[0] != 'Y' && res[0] != 'y') {
            printf("Sauvegarde annulée.\n");

            return DA_BRUTE_DONE;
        }
        return DA_BRUTE_INTERRUPTED;
    }

    printf("\n");
    fflush(stdout);
    return DA_BRUTE_DONE;
}


void save_thread_states(void) {
    for (int i = 1; i < num_display_threads; i++) {
        if (thread_state[i].count == 0)
            continue;

        char filename[64];
        snprintf(filename, sizeof(filename),"resume_thread_%d.bin", i);

        FILE *f = fopen(filename, "wb");
        if (!f) continue;

        fwrite(&thread_state[i], sizeof(brute_resume_t), 1, f);
        fclose(f);
    }
}




/*void da_bruteforce_mangling(ManglingConfig *config) {

    char caracteres[256];

    int len = build_charset(caracteres, sizeof(caracteres),
                            da_config.attack.charset_preset,
                            da_config.attack.charset_custom);

    if (len < 0) { write_log(LOG_ERROR,"Erreur lors de l'initialisation du charset","brutforce");return;    }

    const int b = len;
    const int length = DA_MAX_LEN;
    const long long total = puissance(len,DA_MAX_LEN);
    const int mangling_itération = DA_GET_ITERATION_OF_MANGLING(da_config.attack.mangling_config);

    #pragma omp parallel
    {
        char word[DA_MAX_LEN+1];
        word[length] = '\0';

    #pragma omp for schedule(static)
        for (long long n = 0; n < total; n++) {

            if (is_password_found()) continue;

            construire_mot_depuis_index(n, word, caracteres, b, length);

            generate_mangled_words(word,config);// le mangling verifie le mot de base...
            //if (n==5100000) printf("%d : %s\n",n,word);

            long long count = da_hash_get_count();
            if (count%10000==0) {
                #pragma omp critical
                {
                    print_progress_bar(count*mangling_itération,total*mangling_itération,word,false);
                }
            }
        }
    }
    char last_word[DA_MAX_LEN+1] = {'\0'};
    if (!is_password_found()) construire_mot_depuis_index(total,last_word,caracteres,b,length);
    else get_found_password(last_word,DA_MAX_LEN+1);
    print_progress_bar(total, total, last_word, true);

    printf("total : %llu\n",total);
}*/


int build_charset(char *out, size_t out_size,
                  da_charset_preset_t preset,
                  const char *custom_charset)
{
    size_t pos = 0;

    switch (preset) {

        case DA_CHARSET_PRESET_DEFAULT:
            // ASCII imprimable (32 à 126)
            for (int c = 32; c <= 126; c++) {
                if (pos + 1 >= out_size) return -1;
                out[pos++] = (char)c;
            }
            break;

        case DA_CHARSET_PRESET_ALPHANUM:
            // A-Z
            for (char c = 'A'; c <= 'Z'; c++) out[pos++] = c;
            // a-z
            for (char c = 'a'; c <= 'z'; c++) out[pos++] = c;
            // 0-9
            for (char c = '0'; c <= '9'; c++) out[pos++] = c;
            break;

        case DA_CHARSET_PRESET_NUMERIC:
            for (char c = '0'; c <= '9'; c++) out[pos++] = c;
            break;

        case DA_CHARSET_PRESET_CUSTOM:
            if (!custom_charset) return -2;
            pos = strlen(custom_charset);
            if (pos + 1 >= out_size) return -3;
            memcpy(out, custom_charset, pos);
            break;

        default:
            return -4;
    }

    out[pos] = '\0';  // terminaison string C
    return pos;       // retourne la taille
}


/*static inline void construire_mot_depuis_index(long long n,char *mot,const char *charset,int b, int max_len)
{
    int longueur = 1;
    long long plage = b;
    long long idx = n;

    // Calcul de la longueur du mot
    while (idx >= plage && longueur < max_len) {
        idx -= plage;
        longueur++;
        plage *= b;
    }
    // piur 95
    //0-95 longeur 1
    // 96-9025 longeur 2
    //9025-857 375 longeur 3
    //857 375-81 450 625 longeur 4

    mot[longueur] = '\0';

    // Construction du mot (base b)
    for (int i = longueur - 1; i >= 0; i--) {
        mot[i] = charset[idx % b];
        idx /= b;
    }
    /* 7809
     * caractere 19.
     * caractere 82.
     * reste 0 donc fin.
     *
    */
//}

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


//a pointage = 0
//b pointage = 0
//c pointage = 0
//aa pointage = 1
//ab pointage = 1
//ac pointage = 1
//ba pointage = 1 Retour arriere
//bb pointage = 1
//bc pointage = 1
//ca pointage = 1
//cb pointage = 1
//cc pointage = 1
//aaa pointage = 2
//aab pointage = 2
//aac pointage = 2
//aba pointage = 2 Retour arriere
//abb pointage = 2
//abc pointage = 2
//aca pointage = 2 Retour arriere
//acb pointage = 2
//acc pointage = 2
//baa pointage = 2 Retour arriere *2
