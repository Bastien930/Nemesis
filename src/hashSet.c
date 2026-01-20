//
// Created by Basti on 27/11/2025.
//

#include "hashSet.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>



/**
 * Calculer le hash d'une chaîne de caractères selon l'algorithme FNV-1a
 * 
 * @param str La chaîne à hasher
 * @return La valeur de hash modulo HASHSET_SIZE
 */
static inline uint32_t hash_string(const char *str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619u;
    }
    return hash % HASHSET_SIZE;
}

/**
 * Initialiser un nouveau HashSet vide
 * 
 * @param hs Le HashSet à initialiser
 */
void hashset_init(HashSet *hs) {
    memset(hs->buckets, 0, sizeof(hs->buckets));
    hs->count = 0;
}

/**
 * Ajouter une chaîne au HashSet
 * 
 * @param hs Le HashSet dans lequel ajouter
 * @param str La chaîne à ajouter
 * @return 1 si la chaîne a été ajoutée, 0 si elle était déjà présente
 */
int hashset_add(HashSet *hs,const char *str) {
    uint32_t idx = hash_string(str);
    HashNode *node = hs->buckets[idx];

    // Chercher si existe déjà
    while (node) {
        if (strcmp(node->key, str) == 0) {
            return 0;  // Déjà présent
        }
        node = node->next;
    }

    // Ajouter nouveau noeud
    HashNode *new_node = malloc(sizeof(HashNode));
    new_node->key = strdup(str);
    new_node->next = hs->buckets[idx];
    hs->buckets[idx] = new_node;
    hs->count++;
    return 1;  // Nouveau
}

/**
 * Libérer la mémoire utilisée par un HashSet
 * 
 * @param hs Le HashSet à libérer
 */
void hashset_free(HashSet *hs) {
    for (int i = 0; i < HASHSET_SIZE; i++) {
        HashNode *node = hs->buckets[i];
        while (node) {
            HashNode *tmp = node;
            node = node->next;
            free(tmp->key);
            free(tmp);
        }
    }
}