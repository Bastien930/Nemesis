//
// Created by bastien on 10/24/25.
//

#ifndef NEMESIS_CONFIG_H
#define NEMESIS_CONFIG_H


#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "Mangling.h"

#define NEMESIS_MAX_PATH 4096
#define CHUNK_SIZE 2048
#define CHUNK_SIZE_MANGLING 2048
#define NEMESIS_MAX_THREADS 256
#define SPEED_PRINT 20000 // ou 20000
#define NEMESIS_MIN_LEN 1
#define NEMESIS_MAX_LEN 4
#define NEMESIS_MAX_ATTEMPTS 10000000
#define NEMESIS_MANGLING_FAST 196
#define NEMESIS_MANGLING_BALANCED 361
#define NEMESIS_MANGLING_AGGRESSIVE 450
#define DICT_BUFFER_SIZE (64 * 1024 * 1024)  // 64 MB buffer
#define NEMESIS_GET_ITERATION_OF_MANGLING(config) \
((config) == NEMESIS_MANGLING_FAST   ? NEMESIS_MANGLING_FAST : \
((config) == NEMESIS_MANGLING_BALANCED ? NEMESIS_MANGLING_BALANCED : \
NEMESIS_MANGLING_AGGRESSIVE))
#define NEMESIS_STOPPED_FILE "NEMESIS_binary.conf"


//#define NEMESIS_MAX_ALLOWED_LEN 64
//#define NEMESIS_MIN_ALLOWED_LEN 1

#define NEMESIS_VERSION 3
#define NEMESIS_CONFIG_MAGIC 0x43464731
#define NEMESIS_BUILD_DATE __DATE__ " " __TIME__
#define NEMESIS_AUTHOR "BASTIEN-ALEXIS-ILIAN"
#define NEMESIS_ERRLEN 2048
#define DIR_OF_EXE "/proc/self/"
#define DIR_OF_APP


typedef enum {
    NEMESIS_OUT_TXT,
    NEMESIS_OUT_CSV,
    NEMESIS_OUT_JSON,
    NEMESIS_OUT_XML
} NEMESIS_output_format_t;

typedef enum {
    NEMESIS_CHARSET_PRESET_DEFAULT,
    NEMESIS_CHARSET_PRESET_ALPHANUM,
    NEMESIS_CHARSET_PRESET_NUMERIC,
    NEMESIS_CHARSET_PRESET_CUSTOM
} NEMESIS_charset_preset_t;

/* Sous-structures */

struct NEMESIS_input {
    char shadow_file[NEMESIS_MAX_PATH];
    char wordlist_file[NEMESIS_MAX_PATH];
    int save;

};

struct NEMESIS_attack {
    bool enable_dictionary;
    bool enable_bruteforce;
    bool enable_mangling;
    int mangling_config;
    NEMESIS_charset_preset_t charset_preset;
    char charset_custom[256];
    int min_len;
    int max_len;
    int max_attempts;
    //int retry_delay_ms;
};

struct NEMESIS_output {
    char output_file[NEMESIS_MAX_PATH];
    NEMESIS_output_format_t format;
    //bool enable_stdout;
    bool enable_logging;
    char log_file[NEMESIS_MAX_PATH];
    char config_dir[NEMESIS_MAX_PATH];
    char log_dir[NEMESIS_MAX_PATH];
    char save_dir[NEMESIS_MAX_PATH];
};

struct NEMESIS_system {
    int threads;
    bool enable_gpu;
};

struct NEMESIS_meta {
    int version;
    const char *build_date;
    const char *author;
};

/* Config principale */
typedef struct {
    struct NEMESIS_input input;
    struct NEMESIS_attack attack;
    struct NEMESIS_output output;
    struct NEMESIS_system system;
    struct NEMESIS_meta meta;

    //bool show_help;
}NEMESIS_config_t;

struct NEMESIS_config_file {
    __uint32_t magic;
    __uint32_t version;
    __uint32_t checksum;

    // Données sérialisables (sans pointeurs)
    struct NEMESIS_input input;
    struct NEMESIS_attack attack;
    struct NEMESIS_output output;
    struct NEMESIS_system system;

    // Meta: seulement la version (pas les pointeurs)
    int meta_version;
};

extern NEMESIS_config_t NEMESIS_config;
extern volatile sig_atomic_t interrupt_requested;
extern char SavePath[128];

void NEMESIS_config_init_default(NEMESIS_config_t *cfg);
int NEMESIS_config_validate(const NEMESIS_config_t *cfg, char *errbuf, size_t errlen);
void NEMESIS_print_usage(const char *progname);
int NEMESIS_save_config(FILE *file,NEMESIS_config_t *config);
int NEMESIS_load_config(NEMESIS_config_t *config) ;
void NEMESIS_safe_save_config() ;

#endif //NEMESIS_CONFIG_H