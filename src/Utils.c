//
// Created by bastien on 10/25/25.
//


#include "Utils.h"
#include "Shadow.h"
#include "Hash_Engine.h"
#include "Config.h"
#include "log.h"


#include <stdio.h>
#include <unistd.h> // pour usleep()
#include <stdatomic.h>
#include <pthread.h>
#include <omp.h>
#include <sys/stat.h>
#include <string.h>

long NEMESIS_iteration = 1;

clock_t start;
clock_t end;
double start_omp;
double end_omp;
atomic_int g_password_found = 0;

char g_found_password[MAX_WORD_LEN] = {0};
pthread_mutex_t g_found_password_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Initialize timing variables for performance measurement
 */
void init_time(void) {
    start = clock();
    start_omp = omp_get_wtime();

}

/**
 * Stop timing measurement and store end values
 */
void end_time(void) {
    end = clock();
    end_omp = omp_get_wtime();

}

/**
 * Store a found password in thread-safe way
 * 
 * @param pw Password string to store
 */
void set_found_password(const char *pw) {
    if (!pw) return;

    pthread_mutex_lock(&g_found_password_lock);
    strncpy(g_found_password, pw, MAX_WORD_LEN - 1);
    g_found_password[MAX_WORD_LEN - 1] = '\0';
    pthread_mutex_unlock(&g_found_password_lock);

    // lever le flag atomiquement
    atomic_store_explicit(&g_password_found, 1, memory_order_release);
}

/**
 * Reset the found password and status flags
 */
void reset_found_password(void) {
    pthread_mutex_lock(&g_found_password_lock);
    g_found_password[0] = '\0';
    pthread_mutex_unlock(&g_found_password_lock);

    // lever le flag atomiquement
    atomic_store_explicit(&g_password_found,0, memory_order_release);
}

/**
 * Check if a password has been found
 *
 * @return true if password was found, false otherwise
 */
bool is_password_found(void) {
    return atomic_load_explicit(&g_password_found, memory_order_acquire);
}

/**
 * Copy the found password into a buffer in thread-safe way
 *
 * @param buf Buffer to store password
 * @param bufsize Size of buffer
 */
void get_found_password(char *buf, size_t bufsize) {
    if (!buf || bufsize == 0) return;

    pthread_mutex_lock(&g_found_password_lock);
    snprintf(buf, bufsize, "%s", g_found_password);
    pthread_mutex_unlock(&g_found_password_lock);
}





/**
 * Print text character by character with delay
 *
 * @param text Text to print
 * @param delay_us Microseconds to wait between characters
 */
void print_slow(const char *text, useconds_t delay_us) {
    for (const char *p = text; *p != '\0'; p++) {
        printf("%c", *p);
        fflush(stdout);
        usleep(delay_us);
    }
}

/**
 * Prépare les statistiques
 */
result_stats_t prepare_result_stats(void) {
    result_stats_t stats = {0};

    get_found_password(stats.password, MAX_WORD_LEN);
    stats.time_omp = end_omp - start_omp;
    stats.time_cpu = ((double)(end - start)) / CLOCKS_PER_SEC;
    stats.password_count = NEMESIS_hash_get_count();
    stats.found = (strlen(stats.password) > 0) ? 1 : 0;

    // Calculs dérivés
    if (stats.time_omp > 0) {
        stats.throughput = stats.password_count / stats.time_omp;
        stats.cpu_ratio = stats.time_cpu / stats.time_omp;
    }

    return stats;
}

/**
 * Affiche les résultats à l'écran
 */
void show_result(struct NEMESIS_shadow_entry* shadow_entry, const result_stats_t* stats) {
    if (!stats->found) {
        PRINT_SLOW_MACRO(SPEED_PRINT, "Aucune correspondance trouvée pour : %s\n",shadow_entry->username);
    } else {
        PRINT_SLOW_MACRO(SPEED_PRINT, "Password trouvé : %s\n pour l'utilisateur : %s\n",stats->password, shadow_entry->username);
    }

    PRINT_SLOW_MACRO(SPEED_PRINT, "Hash : %s | Salt : %s\n",shadow_entry->hash, shadow_entry->salt);
    PRINT_SLOW_MACRO(SPEED_PRINT, "%lu mots testés en %.2f secondes\n",stats->password_count, stats->time_omp);
    PRINT_SLOW_MACRO(SPEED_PRINT, "Temps CPU : %.2fs | Débit : %.2f pwd/s | Ratio : %.2fx\n\n",stats->time_cpu, stats->throughput, stats->cpu_ratio);
}

/**
 * Écrit au format TXT
 */
static void write_result_txt(FILE* f, struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(f, "\n\n========================================\n");
    fprintf(f, "NEMESIS - Résultat de craquage\n");
    fprintf(f, "Date: %s\n", timestamp);
    fprintf(f, "========================================\n\n");

    fprintf(f, "Utilisateur: %s\n", shadow_entry->username);
    fprintf(f, "Statut: %s\n", stats->found ? "TROUVÉ ✓" : "NON TROUVÉ ✗");

    if (stats->found) {
        fprintf(f, "Password: %s\n", stats->password);
    }

    fprintf(f, "\nDétails du hash:\n");
    fprintf(f, "  Hash: %s\n", shadow_entry->hash);
    fprintf(f, "  Salt: %s\n", shadow_entry->salt);
    fprintf(f, "  Algorithme: %s\n", getAlgoString(shadow_entry->algo));

    fprintf(f, "\nStatistiques:\n");
    fprintf(f, "  Mots testés: %lu\n", stats->password_count);
    fprintf(f, "  Temps réel: %.2f secondes\n", stats->time_omp);
    fprintf(f, "  Temps CPU: %.2f secondes\n", stats->time_cpu);
    fprintf(f, "  Débit: %.2f mots/seconde\n", stats->throughput);
    fprintf(f, "  Ratio CPU/Temps: %.2fx\n", stats->cpu_ratio);
    fprintf(f, "\n========================================\n\n");
}

/**
 * Écrit au format JSON
 */
static void write_result_json(FILE* f, struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats, int is_first) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", localtime(&now));

    if (!is_first) {
        fprintf(f, ",\n");
    }

    fprintf(f, "\n\n  {\n");
    fprintf(f, "    \"timestamp\": \"%s\",\n", timestamp);
    fprintf(f, "    \"username\": \"%s\",\n", shadow_entry->username);
    fprintf(f, "    \"found\": %s,\n", stats->found ? "true" : "false");

    if (stats->found) {
        // Échapper les caractères spéciaux dans le password
        fprintf(f, "    \"password\": \"%s\",\n", stats->password);
    }

    fprintf(f, "    \"hash\": {\n");
    fprintf(f, "      \"value\": \"%s\",\n", shadow_entry->hash);
    fprintf(f, "      \"salt\": \"%s\",\n", shadow_entry->salt);
    fprintf(f, "      \"algorithm\": \"%s\"\n", getAlgoString(shadow_entry->algo));
    fprintf(f, "    },\n");

    fprintf(f, "    \"statistics\": {\n");
    fprintf(f, "      \"passwords_tested\": %lu,\n", stats->password_count);
    fprintf(f, "      \"time_real_seconds\": %.2f,\n", stats->time_omp);
    fprintf(f, "      \"time_cpu_seconds\": %.2f,\n", stats->time_cpu);
    fprintf(f, "      \"throughput_per_second\": %.2f,\n", stats->throughput);
    fprintf(f, "      \"cpu_time_ratio\": %.2f\n", stats->cpu_ratio);
    fprintf(f, "    }\n");
    fprintf(f, "  }");
}

/**
 * Écrit au format CSV
 */
static void write_result_csv(FILE* f, struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats, int write_header) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Header si première ligne
    if (write_header) {
        fprintf(f, "Timestamp,Username,Found,Password,Hash,Salt,Algorithm,");
        fprintf(f, "PasswordsTested,TimeReal,TimeCPU,Throughput,CPURatio\n");
    }

    // Échapper les virgules et guillemets dans les champs
    fprintf(f, "\n\n\"%s\",", timestamp);
    fprintf(f, "\"%s\",", shadow_entry->username);
    fprintf(f, "%s,", stats->found ? "YES" : "NO");
    fprintf(f, "\"%s\",", stats->found ? stats->password : "");
    fprintf(f, "\"%s\",", shadow_entry->hash);
    fprintf(f, "\"%s\",", shadow_entry->salt);
    fprintf(f, "\"%s\",", getAlgoString(shadow_entry->algo));
    fprintf(f, "%lu,", stats->password_count);
    fprintf(f, "%.2f,", stats->time_omp);
    fprintf(f, "%.2f,", stats->time_cpu);
    fprintf(f, "%.2f,", stats->throughput);
    fprintf(f, "%.2f\n", stats->cpu_ratio);
}

/**
 * Écrit au format XML (bonus)
 */
static void write_result_xml(FILE* f, struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats, int is_first) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", localtime(&now));

    fprintf(f, "\n\n  <result>\n");
    fprintf(f, "    <timestamp>%s</timestamp>\n", timestamp);
    fprintf(f, "    <username>%s</username>\n", shadow_entry->username);
    fprintf(f, "    <found>%s</found>\n", stats->found ? "true" : "false");

    if (stats->found) {
        fprintf(f, "    <password><![CDATA[%s]]></password>\n", stats->password);
    }

    fprintf(f, "    <hash>\n");
    fprintf(f, "      <value>%s</value>\n", shadow_entry->hash);
    fprintf(f, "      <salt>%s</salt>\n", shadow_entry->salt);
    fprintf(f, "      <algorithm>%s</algorithm>\n", getAlgoString(shadow_entry->algo));
    fprintf(f, "    </hash>\n");

    fprintf(f, "    <statistics>\n");
    fprintf(f, "      <passwords_tested>%lu</passwords_tested>\n", stats->password_count);
    fprintf(f, "      <time_real_seconds>%.2f</time_real_seconds>\n", stats->time_omp);
    fprintf(f, "      <time_cpu_seconds>%.2f</time_cpu_seconds>\n", stats->time_cpu);
    fprintf(f, "      <throughput_per_second>%.2f</throughput_per_second>\n", stats->throughput);
    fprintf(f, "      <cpu_time_ratio>%.2f</cpu_time_ratio>\n", stats->cpu_ratio);
    fprintf(f, "    </statistics>\n");
    fprintf(f, "  </result>\n");
}

/**
 * Écrit les résultats dans un fichier au format spécifié.
 *
 * @param shadow_entry Pointeur vers une entrée de type NEMESIS_shadow_entry contenant les informations à inclure dans le résultat.
 * @param stats Pointeur vers une structure de statistique de type result_stats_t contenant les statistiques calculées.
 * @param output_file Nom du fichier où écrire les résultats. Si le fichier n'existe pas, il sera créé.
 * @param format Format de sortie à utiliser. Les formats supportés sont définis dans NEMESIS_output_format_t.
 */
void write_result(struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats,const char* output_file,NEMESIS_output_format_t format) {

    if (!output_file || output_file[0] == '\0') {
        write_log(LOG_WARNING, "Aucun fichier de sortie spécifié", "write_result");
        return;
    }

    // Déterminer le mode d'ouverture
    const char* mode = "a";  // Append par défaut
    int write_header = 0;
    int is_first = 0;

    // Pour CSV, vérifier si le fichier existe
    if (format == NEMESIS_OUT_CSV) {
        FILE* test = fopen(output_file, "r");
        if (!test) {
            mode = "w";
            write_header = 1;
        } else {
            fclose(test);
        }
    }

    // Pour JSON et XML, gérer la structure
    if (format == NEMESIS_OUT_JSON || format == NEMESIS_OUT_XML) {
        FILE* test = fopen(output_file, "r");
        if (!test) {
            mode = "w";
            is_first = 1;
        } else {
            // Vérifier si le fichier est vide
            fseek(test, 0, SEEK_END);
            long size = ftell(test);
            is_first = (size == 0);
            fclose(test);

            if (!is_first && format == NEMESIS_OUT_JSON) {
                // Retirer le dernier ]\n pour ajouter un nouvel élément
                FILE* f = fopen(output_file, "r+");
                if (f) {
                    fseek(f, -2, SEEK_END);
                    ftruncate(fileno(f), ftell(f));
                    fclose(f);
                }
            } else if (!is_first && format == NEMESIS_OUT_XML) {
                // Retirer le dernier </results>\n
                FILE* f = fopen(output_file, "r+");
                if (f) {
                    fseek(f, -11, SEEK_END);
                    ftruncate(fileno(f), ftell(f));
                    fclose(f);
                }
            }
        }
    }

    FILE* f = fopen(output_file, mode);
    if (!f) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Impossible d'ouvrir le fichier: %s", output_file);
        write_log(LOG_ERROR, error_msg, "write_result");
        return;
    }

    // Écrire selon le format
    switch (format) {
        case NEMESIS_OUT_TXT :
            write_result_txt(f, shadow_entry, stats);
            break;

        case NEMESIS_OUT_JSON:
            if (is_first) {
                fprintf(f, "[\n");
            }
            write_result_json(f, shadow_entry, stats, !is_first && ftell(f) > 2);
            fprintf(f, "\n]\n");
            break;

        case NEMESIS_OUT_CSV:
            write_result_csv(f, shadow_entry, stats, write_header);
            break;

        case NEMESIS_OUT_XML:
            if (is_first) {
                fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
                fprintf(f, "<results>\n");
            }
            write_result_xml(f, shadow_entry, stats, is_first);
            printf("result xml\n");
            fprintf(f, "</results>\n");
            break;

        default:
            write_log(LOG_ERROR, "Format de sortie inconnu", "write_result");
            break;
    }

    fclose(f);

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Résultats écrits dans %s", output_file);
    write_log(LOG_INFO, log_msg, "write_result");
}

/**
 * Fonction générique : affiche ET écrit
 */
void show_and_write_result(struct NEMESIS_shadow_entry* shadow_entry,const char* output_file,NEMESIS_output_format_t format) {
    result_stats_t stats = prepare_result_stats();

    show_result(shadow_entry, &stats);
    write_result(shadow_entry, &stats, output_file, format);
}

/**
 * Safely concatenate two strings into a destination buffer
 *
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer
 * @param s1 First string to concatenate
 * @param s2 Second string to concatenate
 * @return true if concatenation succeeded, false if buffer too small or null params
 */
bool safe_concat(char *dest, size_t dest_size, const char *s1, const char *s2) {
    if (!dest || !s1 || !s2) return false;

    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);

    if (len1 + len2 + 1 > dest_size) {
        return false; // Trop long
    }

    memcpy(dest, s1, len1);
    memcpy(dest + len1, s2, len2);
    dest[len1 + len2] = '\0';

    return true;
}

/**
 * Calculate integer power of a number
 *
 * @param base Base number
 * @param exp Exponent
 * @return base raised to exp power
 */
uint_fast64_t puissance(int base, int exp) {
    long long res = 1;
    for (int i = 0; i < exp; i++) {
        res *= base;
    }
    return res;
}

/**
 * Check if a file exists
 *
 * @param path Path to file
 * @return 1 if file exists, 0 otherwise
 */
int File_exist(const char *path) {
    return access(path, F_OK) == 0;
}


/**
 * Get user input with prompt message
 *
 * @param buff Buffer to store input
 * @param message Prompt message to display
 * @param len Maximum length to read
 * @return 0 on success, -1 on error
 */
int ask_input(char *buff,char *message,size_t len) {
    PRINT_SLOW_MACRO(SPEED_PRINT,"%s",message);
    if (!buff || len == 0)
        return -1;

    if (!fgets(buff, len, stdin))
        return -1;
    return 0;
}


void print_banner(void) {
    printf("\n");

    printf(RED);
    print_slow("                Initializing ", 50000);
    print_slow("NEMESIS_ ", 50000);
    print_slow("modules...\n\n", 50000);
    printf(RESET);
    usleep(300000);

    printf(RED);
    print_slow("    ███╗   ██╗███████╗███╗   ███╗███████╗███████╗██╗███████╗\n", 100);
    usleep(100000);
    print_slow("    ████╗  ██║██╔════╝████╗ ████║██╔════╝██╔════╝██║██╔════╝\n", 100);
    usleep(100000);
    print_slow("    ██╔██╗ ██║█████╗  ██╔████╔██║█████╗  ███████╗██║███████╗\n", 100);
    usleep(100000);
    print_slow("    ██║╚██╗██║██╔══╝  ██║╚██╔╝██║██╔══╝  ╚════██║██║╚════██║\n", 100);
    usleep(100000);
    print_slow("    ██║ ╚████║███████╗██║ ╚═╝ ██║███████╗███████║██║███████║\n", 100);
    usleep(100000);
    print_slow("    ╚═╝  ╚═══╝╚══════╝╚═╝     ╚═╝╚══════╝╚══════╝╚═╝╚══════╝\n", 100);
    usleep(200000);
    printf(RESET);

    printf(RED);
    print_slow("   ──────────────────────────────────────────────────────────\n", 100);
    usleep(80000);
    print_slow("     NEMESIS_ — Dictionary & Bruteforce Attack Tool v1.0    \n", 100);
    usleep(80000);
    print_slow("       Multi-attack | Threads | Resume | Checkpoints   \n", 100);
    usleep(80000);
    print_slow("   ──────────────────────────────────────────────────────────      \n", 100);
    usleep(80000);
    print_slow("             Authors: Bastien | Alexis | Ilian                    \n", 100);
    usleep(80000);
    print_slow("   ──────────────────────────────────────────────────────────     \n\n", 100);
    printf(RESET);

}


/**
 * Ensure a directory exists, creating it if necessary
 *
 * @param path Path to directory
 * @return 0 if directory exists or was created, -1 on error
 */
static int ensure_dir_exists(const char *path)
{
    struct stat st;

    if (stat(path, &st) == 0)
        return S_ISDIR(st.st_mode) ? 0 : -1;

    if (mkdir(path, 0700) == 0)
        return 0;

    return -1;
}


/**
 * Initialize application paths and create required directories
 *
 * @return 0 on success, -1 on error
 */
int nemesis_init_paths(void)
{
    const char *home = getenv("HOME");
    if (!home) return -1;
    char base_dir[NEMESIS_MAX_PATH];
    snprintf(base_dir,NEMESIS_MAX_PATH,"%s/Nemesis",home);

    /* ~/.config */
    snprintf(NEMESIS_config.output.config_dir, NEMESIS_MAX_PATH, "%s/config", base_dir);
    snprintf(NEMESIS_config.output.log_dir,    NEMESIS_MAX_PATH, "%s/logs",   base_dir);
    snprintf(NEMESIS_config.output.save_dir,   NEMESIS_MAX_PATH, "%s/saves",  base_dir);

    /* Création des dossiers */
    if (ensure_dir_exists(home) != 0) return -1;
    if (ensure_dir_exists(base_dir) != 0) return -1;
    if (ensure_dir_exists(NEMESIS_config.output.config_dir) != 0) return -1;
    if (ensure_dir_exists(NEMESIS_config.output.log_dir) != 0) return -1;
    if (ensure_dir_exists(NEMESIS_config.output.save_dir) != 0) return -1;

    return 0;
}



