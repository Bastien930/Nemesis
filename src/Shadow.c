//
// Created by bastien on 10/24/25.
//
#include "../Include/Shadow.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Hash_Engine.h"

static char *strdup_safe(const char *s) {
    if (!s) return NULL;             // protection contre NULL
    size_t n = strlen(s) + 1;       // +1 pour '\0'
    char *p = malloc(n);             // allocation
    if (!p) return NULL;             // malloc failed
    memcpy(p, s, n);                 // copie mémoire complète (y compris '\0')
    return p;
}

struct NEMESIS_shadow_entry *fail_cleanup(struct NEMESIS_shadow_entry *entry, char *line_copy) {
    if (entry) {
        if (entry->username) free(entry->username);
        if (entry->salt) free(entry->salt);
        if (entry->hash) free(entry->hash);
        free(entry);
    }
    if (line_copy) free(line_copy);
    return NULL;
}


/*
 * a chaque fois que tu voudra copier utilise strdup_safe pour just allouer de la mémoire et copier les donnes dans le nouvelle espace memoire.
 * param const char *line : line du fichier shadow
 * utilise strtok pour separer les lignes au $
 * renvoie l'adresse memoire de la structure rempli.
 * ps : le type d'algo est un enum dans le fichier Shadow.h
*/
struct NEMESIS_shadow_entry *NEMESIS_parse_shadow_line(const char *line) {
    if (!line) return NULL;
    char *line_copy = strdup_safe(line);
    if (!line_copy) return NULL;

    struct NEMESIS_shadow_entry *entry = malloc(sizeof(struct NEMESIS_shadow_entry));
    if (!entry) return fail_cleanup(NULL, line_copy);

    entry->salt = NULL;
    entry->hash = NULL;

    char *token = strtok(line_copy, ":");
    if (!token) return fail_cleanup(entry, line_copy);
    entry->username = strdup_safe(token);
    if (!entry->username) return fail_cleanup(entry, line_copy);

    token = strtok(NULL, ":");
    if (!token) return fail_cleanup(entry, line_copy);

    char *hash_copy = strdup_safe(token);
    if (!hash_copy) return fail_cleanup(entry, line_copy);

    //$algo$salt$hash
    //strtok(hash_copy, "$");
    char *p = hash_copy; // car sinon free marche pas
    if (*p=='$') p++;// ignore le premier
    char *algo_str = strtok(p, "$");
    char *salt     = strtok(NULL, "$");
    char *hashval  = strtok(NULL, "$");
    if (!algo_str || !salt || !hashval) {
        free(hash_copy);
        return fail_cleanup(entry, line_copy);
    }
    NEMESIS_hash_algo_t algo = getAlgo(algo_str);
    if (!isAlgoImplemented(algo)) {
        free(hash_copy);
        return fail_cleanup(entry, line_copy);
    }

    char tmp[256];
    snprintf(tmp, sizeof(tmp), "$%s$%s$", algo_str, salt);
    entry->salt = strdup_safe(tmp);

    entry->algo = algo;
    entry->hash = strdup_safe(hashval);

    free(hash_copy);
    free(line_copy);

    return entry;
}

NEMESIS_hash_algo_t getAlgo(char *str) {
    if (!str) return NEMESIS_HASH_UNKNOWN;

    const char *md5_prefix      = "1";
    const char *sha256_prefix   = "5";
    const char *sha512_prefix   = "6";
    const char *yescrypt_prefix   = "y";
    const char *bcrypt2a_prefix = "2a";
    const char *bcrypt2b_prefix = "2b";
    const char *bcrypt2y_prefix = "2y";
    const char *argon2i_prefix  = "argon2i";
    const char *argon2id_prefix = "argon2id";

    if (strcmp(str, md5_prefix) == 0) {
        return NEMESIS_HASH_MD5;
    } else if (strcmp(str, sha256_prefix) == 0) {
        return NEMESIS_HASH_SHA256;
    } else if (strcmp(str, sha512_prefix) == 0) {
        return NEMESIS_HASH_SHA512;
    } else if (strcmp(str, yescrypt_prefix) == 0) {
        return NEMESIS_HASH_YESCRYPT;
    } else if (strcmp(str, bcrypt2a_prefix) == 0 ||
               strcmp(str, bcrypt2b_prefix) == 0 ||
               strcmp(str, bcrypt2y_prefix) == 0) {
        return NEMESIS_HASH_BCRYPT;
               } else if (strcmp(str, argon2i_prefix) == 0) {
                   return NEMESIS_HASH_ARGON2I;
               } else if (strcmp(str, argon2id_prefix) == 0) {
                   return NEMESIS_HASH_ARGON2ID;
               }

    return NEMESIS_HASH_UNKNOWN;
}

const char* getAlgoString(NEMESIS_hash_algo_t algo) {
    switch (algo) {
        case NEMESIS_HASH_MD5:
            return "MD5";

        case NEMESIS_HASH_SHA256:
            return "SHA-256";

        case NEMESIS_HASH_SHA512:
            return "SHA-512";

        case NEMESIS_HASH_YESCRYPT:
            return "yescrypt";

        case NEMESIS_HASH_BCRYPT:
            return "bcrypt";

        case NEMESIS_HASH_ARGON2I:
            return "Argon2i";

        case NEMESIS_HASH_ARGON2ID:
            return "Argon2id";

        case NEMESIS_HASH_UNKNOWN:
        default:
            return "Unknown";
    }
}


bool isAlgoImplemented(NEMESIS_hash_algo_t algo) {
    switch(algo) {
        case NEMESIS_HASH_MD5:
            return ENABLE_MD5;
        case NEMESIS_HASH_SHA256:
            return ENABLE_SHA256;
        case NEMESIS_HASH_SHA512:
            return ENABLE_SHA512;
        case NEMESIS_HASH_BCRYPT:
            return ENABLE_BCRYPT;
        case NEMESIS_HASH_YESCRYPT:
            return ENABLE_YESCRYPT;
        case NEMESIS_HASH_ARGON2I:
            return ENABLE_ARGON2I;
        case NEMESIS_HASH_ARGON2ID:
            return ENABLE_ARGON2ID;
        default:
            return 0;
    }
}


    /*
 * libere les champs de la strcuture passer en parametre.
 * libere la structure passer en parametre.
 * return : void.
*/
void NEMESIS_free_shadow_entry(struct NEMESIS_shadow_entry *to_free){
    if (!to_free) return;
    if (to_free->username) free(to_free->username);
    if (to_free->salt) free(to_free->salt);
    if (to_free->hash) free(to_free->hash);
    free(to_free);
}
