//
// Created by bastien on 10/24/25.
//


#ifndef DA_SHADOW_H
#define DA_SHADOW_H

#include <stdbool.h>

/* Algorithme du hash */
typedef enum {
    DA_HASH_UNKNOWN = 0,
    DA_HASH_MD5,
    DA_HASH_SHA256,
    DA_HASH_SHA512,
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

/* Libération mémoire */
void da_free_shadow_entry(struct da_shadow_entry *to_free);

/* Parse ligne shadow et retourne une struct remplie */
struct da_shadow_entry *da_parse_shadow_line(const char *line);

#endif /* DA_SHADOW_H */