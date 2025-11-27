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

#include <stdio.h>
#include <unistd.h> // pour usleep()

#define RED   "\x1b[31m"
#define RESET "\x1b[0m"

#include <string.h>

long da_iteration = 1;

/* définition des variables globales */
atomic_int g_password_found = 0;

char g_found_password[MAX_WORD_LEN] = {0};
pthread_mutex_t g_found_password_lock = PTHREAD_MUTEX_INITIALIZER;

void set_found_password(const char *pw) {
    if (!pw) return;

    pthread_mutex_lock(&g_found_password_lock);
    strncpy(g_found_password, pw, MAX_WORD_LEN - 1);
    g_found_password[MAX_WORD_LEN - 1] = '\0';
    pthread_mutex_unlock(&g_found_password_lock);

    // lever le flag atomiquement
    atomic_store_explicit(&g_password_found, 1, memory_order_release);
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
