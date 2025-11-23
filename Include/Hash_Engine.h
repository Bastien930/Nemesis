#ifndef HASH_ENGINE_H
#define HASH_ENGINE_H

#include <stdbool.h>
#include "Shadow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Utils.h"

/* ============================================
   HIGH-PERFORMANCE HASH ENGINE
   ============================================ */

/* Signature for comparison function pointer */
typedef bool (*da_hash_compare_fn)(const char *password);

/* ========== GLOBAL CONSTANTS (defined in Hash_Engine.c) ========== */
extern const char *da_hash;    /* target hash in hex */
extern const char *da_salt;           /* salt in hex */
extern unsigned long da_hash_count;   /* number of hashes calculated */
extern da_hash_compare_fn da_compare_fn; /* pointer to active compare function */

/* ========== INLINE FUNCTIONS FOR MAX PERFORMANCE ========== */

/**
 * Compare a password with the target hash
 * Inline for maximum speed in brute-force loops
 */
static inline bool da_hash_compare(const char *password) {
    da_hash_count++;
    //printf("%s\n",password);
    //return da_compare_fn(password);
    if ( da_compare_fn(password)) {
        set_found_password(password);
        return true;
    }
    return false;

}

/**
 * Get the number of hashes calculated
 */
static inline long da_hash_get_count(void) {
    return da_hash_count;
}

/**
 * Reset the hash counter
 */
static inline void da_hash_reset_count(void) {
    da_hash_count = 0;
}

/**
 * Check if the hash engine is initialized
 */
static inline bool da_hash_is_initialized(void) {
    return da_compare_fn != NULL;
}

/* ========== PUBLIC API ========== */

/**
 * Initialize the hash engine with a shadow entry
 * Sets up all globals for maximum performance
 * @param entry Parsed shadow entry (must remain valid during attack)
 * @return true on success, false on failure
 */
bool da_hash_engine_init(struct da_shadow_entry *entry);

/**
 * Reset the hash engine and free internal buffers
 */
void da_hash_engine_reset(void);

void free_global_sha() ;

void init_global_sha() ;

bool sha256_compare_fn(const char *password);


#endif /* HASH_ENGINE_H */
