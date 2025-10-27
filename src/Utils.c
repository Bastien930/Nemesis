//
// Created by bastien on 10/25/25.
//


#include "Utils.h"

#include <stdio.h>
#include <unistd.h> // pour usleep()

#include <stdio.h>
#include <unistd.h> // pour usleep()

#define RED   "\x1b[31m"
#define RESET "\x1b[0m"

static inline bool da_hash_into(const char *input, enum da_hash_algo algo, char *out, int out_size)
{
    ;
}

static inline bool da_hash_matches(const char *candidate, const char *target_hash, enum da_hash_algo algo)
{
    ;
}




void print_slow(const char *text, useconds_t delay_us) {
    for (const char *p = text; *p != '\0'; p++) {
        printf("%c", *p);
        fflush(stdout);
        usleep(delay_us);
    }
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
