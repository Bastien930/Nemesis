//
// Created by bastien on 10/25/25.
//

#ifndef DA_UTILS_H
#define DA_UTILS_H

void print_banner(void);

// hash input et le met dans out grace a une fonction de copy.
static inline bool da_hash_into(const char *input, enum da_hash_algo algo, char *out, int out_size) ;

//static inline bool da_hash_matches(const char *candidate, const char *target_hash, enum da_hash_algo algo, char *buffer, int buffer_size);

//hash le candidat et le compare avec target_hash
static inline bool da_hash_matches(const char *candidate, const char *target_hash, enum da_hash_algo algo);

#endif //DA_UTILS_H