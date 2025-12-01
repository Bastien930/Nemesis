//
// Created by Basti on 27/11/2025.
//

#ifndef DA_HASHSET_H
#define DA_HASHSET_H

#define HASHSET_SIZE 65536  // 2^16, ajustez selon vos besoins

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


#endif //DA_HASHSET_H