//
// Created by bastien on 10/25/25.
//


#include "Utils.h"
#include "Shadow.h"
#include <stdio.h>
#include <unistd.h> // pour usleep()
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>
#include "Hash_Engine.h"
#include "Config.h"
#include <omp.h>

#include <stdio.h>
#include <unistd.h> // pour usleep()

#define RED   "\x1b[31m"
#define RESET "\x1b[0m"

#include <string.h>

#include "main.h"

long da_iteration = 1;

clock_t start;
double start_omp;
/* définition des variables globales */
atomic_int g_password_found = 0;

char g_found_password[MAX_WORD_LEN] = {0};
pthread_mutex_t g_found_password_lock = PTHREAD_MUTEX_INITIALIZER;

void init_time(void) {
    start = clock();
    start_omp = omp_get_wtime();

}

void print_progress_bar(long long current, long long total, const char *word,bool is_slow) {
    const int bar_width = 80;
    float progress = (float)current / total;
    int filled = (int)(progress * bar_width);

    printf("\r[");
    for (int i = 0; i < bar_width; i++) {
        if (is_slow) usleep(30000);
        if (i < filled) printf("=");
        else if (i == filled) printf(">");
        else printf(" ");
        fflush(stdout);
    }
    printf("] %.1f%% (%lld/%lld) | %s",
           progress * 100, current, total, word);
    fflush(stdout);
    if (is_slow) sleep(2);
}


void set_found_password(const char *pw) {
    if (!pw) return;

    pthread_mutex_lock(&g_found_password_lock);
    strncpy(g_found_password, pw, MAX_WORD_LEN - 1);
    g_found_password[MAX_WORD_LEN - 1] = '\0';
    pthread_mutex_unlock(&g_found_password_lock);

    // lever le flag atomiquement
    atomic_store_explicit(&g_password_found, 1, memory_order_release);
}

void reset_found_password(void) {
    pthread_mutex_lock(&g_found_password_lock);
    g_found_password[0] = '\0';
    pthread_mutex_unlock(&g_found_password_lock);

    // lever le flag atomiquement
    atomic_store_explicit(&g_password_found,0, memory_order_release);
}

bool is_password_found(void) {
    return atomic_load_explicit(&g_password_found, memory_order_acquire);
}

// Lecture sécurisée du mot de passe
void get_found_password(char *buf, size_t bufsize) {
    if (!buf || bufsize == 0) return;

    pthread_mutex_lock(&g_found_password_lock);
    snprintf(buf, bufsize, "%s", g_found_password);
    pthread_mutex_unlock(&g_found_password_lock);
}





void print_slow(const char *text, useconds_t delay_us) {
    for (const char *p = text; *p != '\0'; p++) {
        printf("%c", *p);
        fflush(stdout);
        usleep(delay_us);
    }
}

long count_iteration(int iteration) {
    ;
}

void show_result(struct da_shadow_entry* shadow_entry) {
    char buff[MAX_WORD_LEN];
    get_found_password(buff,MAX_WORD_LEN);
    double temp_omp = omp_get_wtime() - start_omp;
    double temp_cpu = ((double)(clock() - start)) / CLOCKS_PER_SEC;

    if (strlen(buff)==0) printf("aucune correspondance a été trouvé... pour l'utilisateur : %s\n",shadow_entry->username);
    else printf("le password trouve est : %s pour l'utilisateur : %s\n",buff,shadow_entry->username);
    printf("Le hash correpondant est : %s avec pour salt : %s\n",shadow_entry->hash,shadow_entry->salt);
    printf("%lu mots de passe testé en : %.2f secondes\n", da_hash_get_count(),temp_omp);
    printf("Temps réel écoulé : %.2f secondes\n", temp_omp);
    printf("Temps CPU cumulé : %.2f secondes\n", temp_cpu);
    printf("Débit réel : %.2f mots de passe/seconde\n", da_hash_get_count() / temp_omp);
    printf("Ratio CPU/Temps réel : %.2fx\n\n", temp_cpu / temp_omp);

}

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

long long puissance(int base, int exp) {
    long long res = 1;
    for (int i = 0; i < exp; i++) {
        res *= base;
    }
    return res;
}

int File_exist(const char *path) {
    return access(path, F_OK) == 0;
}


int ask_input(char *buff,char *message,size_t len) {
    printf("%s",message);
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
    print_slow("NEMESIS ", 50000);
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
    print_slow("     NEMESIS — Dictionary & Bruteforce Attack Tool v1.0    \n", 100);
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
