#ifndef HASH_ENGINE_H
#define HASH_ENGINE_H

#include <stdbool.h>
#include "Shadow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Utils.h"
#include "hashSet.h"
#include <omp.h>

/* ============================================
   HIGH-PERFORMANCE HASH ENGINE
   ============================================ */

/* Signature for comparison function pointer */
typedef bool (*NEMESIS_hash_compare_fn)(const char *password);

/* ========== GLOBAL CONSTANTS (defined in Hash_Engine.c) ========== */
extern const char *NEMESIS_hash;    /* target hash in hex */
extern const char *NEMESIS_salt;           /* salt in hex */
extern long NEMESIS_hash_count;   /* number of hashes calculated */
//extern NEMESIS_hash_compare_fn NEMESIS_compare_fn; /* pointer to active compare function */

/* ========== INLINE FUNCTIONS FOR MAX PERFORMANCE ========== */

const char *extract_crypt_hash_part(const char *crypt_res) ;
bool NEMESIS_crypt(const char *password) ;

/**
 * Compare a password with the target hash
 * Inline for maximum speed in brute-force loops
 */
static inline bool NEMESIS_hash_compare(const char *password,HashSet *hs) {



    //printf("%s\n",password);
    //return NEMESIS_compare_fn(password);
    if (hs!=NULL && !hashset_add(hs,password)) { // temps gagner de 5s pour yescrypt par mdp // temps execution fnct 3 000 ns
        return false; // il a pas été ajouter car déja présent
    }
#pragma omp atomic update
    NEMESIS_hash_count++;
    if ( NEMESIS_crypt(password)) {
        #pragma omp critical
        set_found_password(password);
        return true;
    }
    return false;

}




/**
 * Get the number of hashes calculated
 */
static long NEMESIS_hash_get_count(void) {
    return NEMESIS_hash_count;
}

/**
 * Reset the hash counter
 */
static inline void NEMESIS_hash_reset_count(void) {
    NEMESIS_hash_count = 0;
}

/**
 * Check if the hash engine is initialized
 */
static inline bool NEMESIS_hash_is_initialized(void) {
    return NEMESIS_hash != NULL;
}

/* ========== PUBLIC API ========== */

/**
 * Initialize the hash engine with a shadow entry
 * Sets up all globals for maximum performance
 * @param entry Parsed shadow entry (must remain valid during attack)
 * @return true on success, false on failure
 */
bool NEMESIS_hash_engine_init(struct NEMESIS_shadow_entry *entry);

/**
 * Reset the hash engine and free internal buffers
 */
void NEMESIS_hash_engine_reset(void);

//void free_global_sha() ;

//void init_global_sha() ;

//bool sha256_compare_fn(const char *password);


#endif /* HASH_ENGINE_H */
