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


/* drapeau atomique indiquant qu'on a trouvé le mot de passe */
extern atomic_int g_password_found;

/* stockage sécurisé du mot de passe trouvé */
extern char g_found_password[MAX_WORD_LEN];
extern pthread_mutex_t g_found_password_lock;

/* utilitaires pour définir/obtenir le mot de passe trouvé */
void set_found_password(const char *pw);    /* safe: store + set flag */
bool is_password_found(void);               /* atomic load */
void get_found_password(char *buf, size_t bufsize) ;


#endif //DA_UTILS_H