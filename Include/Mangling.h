//
// Created by Basti on 10/11/2025.
//

#ifndef DA_MANGLING_H
#define DA_MANGLING_H

// mangling.

typedef struct {
    int priority_level;        // PRIORITY_HIGH, MEDIUM, LOW
    int use_leetspeak;         // LEET_NONE, LEET_BASIC, etc.
    int use_capitalization;    // CAP_NONE, CAP_FIRST, etc.
    int use_numeric_suffixes;  // MANGLE_NONE/ALL
    int use_year_suffixes;     // MANGLE_NONE/ALL
    int use_symbol_suffixes;   // MANGLE_NONE/ALL
    int use_prefixes;          // MANGLE_NONE/ALL
    int use_repetition;        // MANGLE_NONE/ALL
    int use_reverse;           // MANGLE_NONE/ALL
    int use_common_words;      // MANGLE_NONE/ALL
} ManglingConfig;


void generate_mangled_words(const char *base_word, ManglingConfig *config);
void print_usage_example(int nb) ;

ManglingConfig* get_config_aggressive() ;
ManglingConfig* get_config_balanced() ;
ManglingConfig* get_config_fast() ;


#endif //DA_MANGLING_H