//
// Created by bastien on 10/24/25.
//

#ifndef DA_CONFIG_H
#define DA_CONFIG_H


#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "Mangling.h"

#define DA_MAX_PATH 4096
#define CHUNK_SIZE 2048
#define DA_MAX_THREADS 256
#define DA_MIN_LEN 1
#define DA_MAX_LEN 4
#define DA_MAX_ATTEMPTS 10000000
#define DA_MANGLING_FAST 222
#define DA_MANGLING_BALANCED 606
#define DA_MANGLING_AGGRESSIVE 890
#define DA_GET_ITERATION_OF_MANGLING(config) \
((config) == DA_MANGLING_BALANCED   ? DA_MANGLING_FAST : \
((config) == DA_MANGLING_AGGRESSIVE ? DA_MANGLING_BALANCED : \
DA_MANGLING_AGGRESSIVE))
#define DA_STOPPED_FILE "da_binary.conf"


//#define DA_MAX_ALLOWED_LEN 64
//#define DA_MIN_ALLOWED_LEN 1

#define DA_VERSION 3
#define DA_CONFIG_MAGIC 0x43464731
#define DA_BUILD_DATE __DATE__ " " __TIME__
#define DA_AUTHOR "BASTIEN-ALEXIS-ILIAN"
#define DA_ERRLEN 2048
#define DIR_OF_EXE "/proc/self/"


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
    int save;

};

struct da_attack {
    bool enable_dictionary;
    bool enable_bruteforce;
    bool enable_mangling;
    int mangling_config;
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
    int threads;
    bool enable_gpu;
};

struct da_meta {
    int version;
    const char *build_date;
    const char *author;
};

/* Config principale */
typedef struct {
    struct da_input input;
    struct da_attack attack;
    struct da_output output;
    struct da_system system;
    struct da_meta meta;

    //bool show_help;
}da_config_t;

struct da_config_file {
    __uint32_t magic;
    __uint32_t version;
    __uint32_t checksum;

    // Données sérialisables (sans pointeurs)
    struct da_input input;
    struct da_attack attack;
    struct da_output output;
    struct da_system system;

    // Meta: seulement la version (pas les pointeurs)
    int meta_version;
};

extern da_config_t da_config;
extern volatile sig_atomic_t interrupt_requested;
extern char SavePath[128];

void da_config_init_default(da_config_t *cfg);
int da_config_validate(const da_config_t *cfg, char *errbuf, size_t errlen);
void da_print_usage(const char *progname);
int da_save_config(FILE *file,da_config_t *config);
int da_load_config(da_config_t *config) ;
void da_safe_save_config() ;

#endif //DA_CONFIG_H