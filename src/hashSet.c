//
// Created by Basti on 27/11/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "hashSet.h"



// Hash function FNV-1a (rapide et bonne distribution)
static inline uint32_t hash_string(const char *str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619u;
    }
    return hash % HASHSET_SIZE;
}

void hashset_init(HashSet *hs) {
    memset(hs->buckets, 0, sizeof(hs->buckets));
    hs->count = 0;
}

// Retourne 1 si ajouté (nouveau), 0 si déjà présent
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

    // Ajouter nouveau nœud
    HashNode *new_node = malloc(sizeof(HashNode));
    new_node->key = strdup(str);
    new_node->next = hs->buckets[idx];
    hs->buckets[idx] = new_node;
    hs->count++;
    return 1;  // Nouveau
}

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