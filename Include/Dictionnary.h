//
// Created by Basti on 30/12/2025.
//

#ifndef NEMESIS__DICTIONNARY_H
#define NEMESIS__DICTIONNARY_H
#include "brute_force.h"

NEMESIS_brute_status_t NEMESIS_dictionary_attack(const char *dict_filename) ;
void save_dict_thread_states(void) ;
void delete_dict_thread_states(void) ;
#endif //NEMESIS__DICTIONNARY_H