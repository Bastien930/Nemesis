//
// Created by bastien on 10/24/25.
//

#ifndef DA_BRUTE_FORCE_H
#define DA_BRUTE_FORCE_H

#include "Config.h"

int fctapelante();

void bruteforce(void) ;
#include <stdio.h>
#include <string.h>

int build_charset(char *out, size_t out_size,
                  da_charset_preset_t preset,
                  const char *custom_charset);

#endif //DA_BRUTE_FORCE_H