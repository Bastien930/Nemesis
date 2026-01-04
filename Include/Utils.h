//
// Created by bastien on 10/25/25.
//

#ifndef NEMESIS_UTILS_H
#define NEMESIS_UTILS_H

#include <stdint.h>
#include <unistd.h>

#include "Config.h"
#include "Shadow.h"

void print_banner(void);

#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_WORD_LEN 256
#define LL(x) (long long)(x)

#define STR2(x) #x
#define STR(x) STR2(x) // macro préprocesseur

#define PRINT_SLOW_MACRO(delay, fmt, ...) do { \
char __buf[512]; \
snprintf(__buf, sizeof(__buf), fmt, ##__VA_ARGS__); \
print_slow(__buf, delay); \
} while(0)

#define PATH_JOIN(out, out_size, dir, file) \
snprintf((out), (out_size), "%s/%s", (dir), (file))

typedef struct {
    uint_fast64_t count;
    char last_save_word[NEMESIS_MAX_LEN + 1];
    int active;
} thread_progress_t ;

extern long NEMESIS_iteration;
/* drapeau atomique indiquant qu'on a trouvé le mot de passe */
extern atomic_int g_password_found;

typedef struct {
    char password[MAX_WORD_LEN];
    double time_omp;
    double time_cpu;
    uint64_t password_count;
    double throughput;
    double cpu_ratio;
    int found;  // 1 si trouvé, 0 sinon
} result_stats_t;

/* stockage sécurisé du mot de passe trouvé */
extern char g_found_password[MAX_WORD_LEN];
extern pthread_mutex_t g_found_password_lock;

/* utilitaires pour définir/obtenir le mot de passe trouvé */
void set_found_password(const char *pw);    /* safe: store + set flag */
bool is_password_found(void);               /* atomic load */
void get_found_password(char *buf, size_t bufsize) ;
void init_time(void);
void end_time(void);
bool safe_concat(char *dest, size_t dest_size, const char *s1, const char *s2) ;
void reset_found_password(void) ;

long count_iteration(int iteration);
uint_fast64_t puissance(int base, int exp);
void print_slow(const char *text, useconds_t delay_us) ;
void print_progress_bar(long long current, long long total, const char *word,bool is_slow) ;
int File_exist(const char *path) ;
int ask_input(char *buff, char *message,size_t len) ;
int NEMESIS_need_save() ;

void show_result(struct NEMESIS_shadow_entry* shadow_entry, const result_stats_t* stats) ;
void write_result(struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats,const char* output_file,NEMESIS_output_format_t format) ;
void show_and_write_result(struct NEMESIS_shadow_entry* shadow_entry,const char* output_file,NEMESIS_output_format_t format) ;
static int ensure_dir_exists(const char *path);
int nemesis_init_paths(void);
static void write_result_txt(FILE* f, struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats) ;
static void write_result_json(FILE* f, struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats, int is_first) ;
static void write_result_csv(FILE* f, struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats, int write_header) ;
static void write_result_xml(FILE* f, struct NEMESIS_shadow_entry* shadow_entry,const result_stats_t* stats, int is_first) ;


#endif //NEMESIS_UTILS_H