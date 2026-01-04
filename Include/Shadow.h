//
// Created by bastien on 10/24/25.
//


#ifndef NEMESIS_SHADOW_H
#define NEMESIS_SHADOW_H

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
    NEMESIS_HASH_UNKNOWN = 0,
    NEMESIS_HASH_MD5,
    NEMESIS_HASH_SHA256,
    NEMESIS_HASH_SHA512,
    NEMESIS_HASH_YESCRYPT,
    NEMESIS_HASH_BCRYPT,
    NEMESIS_HASH_ARGON2I,
    NEMESIS_HASH_ARGON2ID
} NEMESIS_hash_algo_t;



/* Entrée shadow simplifiée */
struct NEMESIS_shadow_entry {
    char *username;  /* username (malloc) */
    char *hash;      /* hash complet (malloc) */
    char *salt;      /* salt extrait (malloc) */
    NEMESIS_hash_algo_t algo; /* algorithme utilisé */
};

bool isAlgoImplemented(NEMESIS_hash_algo_t algo) ;

NEMESIS_hash_algo_t getAlgo(char *str) ;

const char* getAlgoString(NEMESIS_hash_algo_t algo) ;
/* Libération mémoire */
void NEMESIS_free_shadow_entry(struct NEMESIS_shadow_entry *to_free);

/* Parse ligne shadow et retourne une struct remplie */
struct NEMESIS_shadow_entry *NEMESIS_parse_shadow_line(const char *line);

#endif /* NEMESIS_SHADOW_H */