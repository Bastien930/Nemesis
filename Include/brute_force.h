//
// Created by bastien on 10/24/25.
//

#ifndef NEMESIS_BRUTE_FORCE_H
#define NEMESIS_BRUTE_FORCE_H

#include "Config.h"
#include "Mangling.h"

typedef enum {
    NEMESIS_BRUTE_DONE,
    NEMESIS_BRUTE_INTERRUPTED,
    NEMESIS_BRUTE_ERROR
} NEMESIS_brute_status_t;

NEMESIS_brute_status_t NEMESIS_bruteforce(void) ;
NEMESIS_brute_status_t NEMESIS_bruteforce_mangling(ManglingConfig config) ;
void save_brute_thread_states(void);
void delete_brut_thread_states(void) ;
#include <stdio.h>
#include <string.h>



int build_charset(char *out, size_t out_size,NEMESIS_charset_preset_t preset,const char *custom_charset);

#endif //NEMESIS_BRUTE_FORCE_H