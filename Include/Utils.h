//
// Created by bastien on 10/25/25.
//

#ifndef DA_UTILS_H
#define DA_UTILS_H

#include "Shadow.h"

void print_banner(void);

#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_WORD_LEN 256

extern long da_iteration;
extern clock_t start;

/* drapeau atomique indiquant qu'on a trouvé le mot de passe */
extern atomic_int g_password_found;

/* stockage sécurisé du mot de passe trouvé */
extern char g_found_password[MAX_WORD_LEN];
extern pthread_mutex_t g_found_password_lock;

/* utilitaires pour définir/obtenir le mot de passe trouvé */
void set_found_password(const char *pw);    /* safe: store + set flag */
bool is_password_found(void);               /* atomic load */
void get_found_password(char *buf, size_t bufsize) ;
void show_result(struct da_shadow_entry* shadow_entry);
void init_time(void);
bool safe_concat(char *dest, size_t dest_size, const char *s1, const char *s2) ;
void reset_found_password(void) ;

long count_iteration(int iteration);
long long puissance(int base, int exp);
void print_progress_bar(long long current, long long total, const char *word,bool is_slow) ;
int File_exist(const char *path) ;
int ask_input(char *buff, char *message,size_t len) ;
int da_need_save() ;

#endif //DA_UTILS_H