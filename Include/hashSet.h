//
// Created by Basti on 27/11/2025.
//

#ifndef NEMESIS_HASHSET_H
#define NEMESIS_HASHSET_H

#define HASHSET_SIZE 512
#include <stddef.h>

typedef struct HashNode {
    char *key;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode *buckets[HASHSET_SIZE];
    size_t count;
} HashSet;


void hashset_free(HashSet *hs) ;
int hashset_add(HashSet *hs,const char *str) ;
void hashset_init(HashSet *hs) ;
void hashset_reset(HashSet *hs) ;

#endif //NEMESIS_HASHSET_H