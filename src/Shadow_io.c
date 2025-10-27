//
// Created by bastien on 10/26/25.
//
#include <stdlib.h>

#include "Shdow_io.h"

struct da_shadow_entry_list da_shadow_entry_liste = {.entries=NULL,.count=0,.capacity=0};



void da_init_shadow_entry_list(struct da_shadow_entry_list *list) {
    list->count = 0;
    list->capacity = 16;
    list->entries = malloc(sizeof(struct da_shadow_entry *) * list->capacity);
}

/*
 * ouvre et ferme un fichier apres avoir lu chacune des lignes, pour chaque ligne appeller struct da_shadow_entry *da_parse_shadow_line(const char *line){
 * et pour ajouter la ligne lu une fois traiter par da_parse_shadiw_line utilise bool da_add_shadow_entry(struct da_shadow_entry *to_add);
*/
int da_load_shadow_file(const char* path,struct da_shadow_entry_list *list)
{
    ;
}

/*
 * ajoute une shadow_entryu a la liste si jamais l'attibut entries de la liste n'a pas assez de place donc count==capacity alors
 * utilise realoc pour reallouer la memoire apres avoir douybler la capaciter.
*/
bool da_add_shadow_entry(struct da_shadow_entry *to_add)
{
    ;
}

/*
 * libere la mamoire allouer pour la structure ainsi que pour sont attribut entries pour cela appeller void da_free_shadow_entry(struct da_shadow_entry *to_free){
 * pour chacune des entrées.
*/
void da_free_shadow_entry_list(struct da_shadow_entry_list *to_free)
{
    ;
}