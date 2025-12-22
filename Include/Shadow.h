//
// Created by bastien on 10/24/25.
//


#ifndef DA_SHADOW_H
#define DA_SHADOW_H

#define ENABLE_MD5       true
#define ENABLE_SHA256    true
#define ENABLE_SHA512    true
#define ENABLE_YESCRYPT    true
#define ENABLE_BCRYPT    false
#define ENABLE_ARGON2I   false
#define ENABLE_ARGON2ID  false
#include <stdbool.h>

/* Algorithme du hash */
typedef enum {
    DA_HASH_UNKNOWN = 0,
    DA_HASH_MD5,
    DA_HASH_SHA256,
    DA_HASH_SHA512,
    DA_HASH_YESCRYPT,
    DA_HASH_BCRYPT,
    DA_HASH_ARGON2I,
    DA_HASH_ARGON2ID
} da_hash_algo_t;



/* Entrée shadow simplifiée */
struct da_shadow_entry {
    char *username;  /* username (malloc) */
    char *hash;      /* hash complet (malloc) */
    char *salt;      /* salt extrait (malloc) */
    da_hash_algo_t algo; /* algorithme utilisé */
};

bool isAlgoImplemented(da_hash_algo_t algo) ;

da_hash_algo_t getAlgo(char *str) ;

/* Libération mémoire */
void da_free_shadow_entry(struct da_shadow_entry *to_free);

/* Parse ligne shadow et retourne une struct remplie */
struct da_shadow_entry *da_parse_shadow_line(const char *line);

#endif /* DA_SHADOW_H */