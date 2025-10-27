//
// Created by bastien on 10/24/25.
//

#ifndef DA_CONFIG_H
#define DA_CONFIG_H


#include <stdbool.h>
#include <stdio.h>

#define DA_MAX_PATH 4096
#define DA_MAX_THREADS 1024
#define DA_MIN_LEN 1
#define DA_MAX_LEN 7
#define DA_MAX_ATTEMPTS 10000000

#define DA_MAX_ALLOWED_LEN 64
#define DA_MIN_ALLOWED_LEN 1

#define DA_VERSION "0.1"
#define DA_BUILD_DATE __DATE__ " " __TIME__
#define DA_AUTHOR "BASTIEN-ALEXIS-ILIAN"
#define DA_ERRLEN 2048

#define STR2(x) #x
#define STR(x) STR2(x) // macro préprocesseur

typedef enum {
    DA_OUT_AUTO,
    DA_OUT_TXT,
    DA_OUT_CSV
} da_output_format_t;

typedef enum {
    DA_CHARSET_PRESET_DEFAULT,
    DA_CHARSET_PRESET_ALPHANUM,
    DA_CHARSET_PRESET_NUMERIC,
    DA_CHARSET_PRESET_CUSTOM
} da_charset_preset_t;

/* Sous-structures */

struct da_input {
    char shadow_file[DA_MAX_PATH];
    char wordlist_file[DA_MAX_PATH];

};

struct da_attack {
    bool enable_dictionary;
    bool enable_bruteforce;
    bool enable_mangling;
    char mangling_rules[512];
    da_charset_preset_t charset_preset;
    char charset_custom[256];
    int min_len;
    int max_len;
    int max_attempts;
    //int retry_delay_ms;
};

struct da_output {
    char output_file[DA_MAX_PATH];
    da_output_format_t format;
    //bool enable_stdout;
    bool enable_logging;
    char log_file[DA_MAX_PATH];
};

struct da_system {
    unsigned int threads;
    unsigned int max_threads;
    bool enable_gpu;
};

struct da_meta {
    const char *version;
    const char *build_date;
    const char *author;
};

/* Config principale */
struct da_config_t {
    struct da_input input;
    struct da_attack attack;
    struct da_output output;
    struct da_system system;
    struct da_meta meta;

    //bool show_help;
};

extern struct da_config_t da_config;

void da_config_init_default(struct da_config_t *cfg);
int da_config_validate(const struct da_config_t *cfg, char *errbuf, size_t errlen);
void da_print_usage(const char *progname);


#endif //DA_CONFIG_H