//
// Created by bastien on 10/24/25.
//

#ifndef DA_OPTION_H
#define DA_OPTION_H

#include <getopt.h>

#include "Config.h"

enum {
    OPT_MIN = 512, // commence a 512 pour pas etre des valeurs assci.
    OPT_MAX,
    OPT_FORMAT,
    OPT_LOG,
    OPT_ENABLE_GPU,
    OPT_MAX_ATTEMPTS,
    OPT_VERSION
};

/* options courtes acceptées */
static const char *short_options = "s:w:m:dbc:t:o:h";

/* tableau des options longues (getopt_long) */
static struct option long_options[] = {
    /* name           has_arg          flag  val */
    {"shadow",        required_argument, 0, 's'},          /* -s, --shadow <file> */
    {"wordlist",      required_argument, 0, 'w'},          /* -w, --wordlist <file> */
    {"mangling",      optional_argument, 0, 'm'},          /* -m, --mangling <rules> */
    {"dictionary",    no_argument,       0, 'd'},          /* -d, --dictionary */
    {"bruteforce",    no_argument,       0, 'b'},          /* -b, --bruteforce */
    {"charset",       required_argument, 0, 'c'},          /* -c, --charset <preset|chars> */
    {"min",           required_argument, 0, OPT_MIN},      /* --min <n> */
    {"max",           required_argument, 0, OPT_MAX},      /* --max <n> */
    {"threads",       required_argument, 0, 't'},          /* -t, --threads <n> */
    {"output",        required_argument, 0, 'o'},          /* -o, --output <file> */
    {"format",        required_argument, 0, OPT_FORMAT},   /* --format <json|csv|txt> */
    {"log",           required_argument, 0, OPT_LOG},      /* --log <file> */
    {"enable-gpu",    no_argument,       0, OPT_ENABLE_GPU}, /* --enable-gpu */
    {"max-attempts",  required_argument, 0, OPT_MAX_ATTEMPTS}, /* --max-attempts <n> */
    {"help",          no_argument,       0, 'h'},          /* -h, --help */
    {"version",       no_argument,       0, OPT_VERSION}, /* --version */
    {0, 0, 0, 0}
};

int parse_args(int argc,char *argv[], struct da_config_t *cfg);

#endif //DA_OPTION_H