//
// Created by bastien on 10/24/25.
//

#ifndef DA_BRUTE_FORCE_H
#define DA_BRUTE_FORCE_H

#include "Config.h"
#include "Mangling.h"

typedef enum {
    DA_BRUTE_DONE,
    DA_BRUTE_INTERRUPTED,
    DA_BRUTE_ERROR
} da_brute_status_t;

da_brute_status_t da_bruteforce(void) ;
void da_bruteforce_mangling(ManglingConfig *config) ;
void save_thread_states(void);
#include <stdio.h>
#include <string.h>



int build_charset(char *out, size_t out_size,da_charset_preset_t preset,const char *custom_charset);

#endif //DA_BRUTE_FORCE_H