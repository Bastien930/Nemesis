//
// Created by bastien on 10/26/25.
//
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#include "log.h"
#include "Shadow.h"
#include "Shdow_io.h"




void NEMESIS_init_shadow_entry_list(struct NEMESIS_shadow_entry_list *list) {
    if (!list) {
        write_log(LOG_ERROR,"erreur de malloc pour NEMESIS_shadow_entry_list","NEMESIS_init_shadow_entry_list()");
        perror("malloc");
    }
    list->count = 0;
    list->capacity = 16;
    list->entries = malloc(sizeof(struct NEMESIS_shadow_entry *) * list->capacity);
    if (!list->entries) {
        list->capacity = 0;
        write_log(LOG_ERROR,"erreur de malloc pour entries","NEMESIS_init_shadow_entry_list()");
        perror("malloc");
    }
}

/*
 * ouvre et ferme un fichier apres avoir lu chacune des lignes, pour chaque ligne appeller struct NEMESIS_shadow_entry *NEMESIS_parse_shadow_line(const char *line){
 * et pour ajouter la ligne lu une fois traiter par NEMESIS_parse_shadiw_line utilise bool NEMESIS_add_shadow_entry(struct NEMESIS_shadow_entry *to_add);
*/
int NEMESIS_load_shadow_file(const char* path,struct NEMESIS_shadow_entry_list *list)
{
    FILE *f = fopen(path,"r");
    char line[1024];

    if (!f) {write_log(LOG_ERROR,"Erreur lors de l'ouverture du fichier shadow.","NEMESIS_load_shadow_file()");perror("fopen");return -1;}

    while (fgets(line,sizeof(line),f)) {
        if (!NEMESIS_add_shadow_entry(NEMESIS_parse_shadow_line(line),list)) {write_log(LOG_WARNING,"Erreur lors de la lecture d'une ligne du ficher shadow.","NEMESIS_load_shadow_file()");}
    }
    fclose(f);

    return list->count;
    ;//renvoie le nombre de ligne pouvant être traiter.
}

/*
 * ajoute une shadow_entryu a la liste si jamais l'attibut entries de la liste n'a pas assez de place donc count==capacity alors
 * utilise realoc pour reallouer la memoire apres avoir douybler la capaciter.
*/
bool NEMESIS_add_shadow_entry(struct NEMESIS_shadow_entry *to_add,struct NEMESIS_shadow_entry_list *list)
{
    if (!to_add || !list) {return false;}
    if (list->count >= list->capacity) {
        int new_capacity = list->capacity ? list->capacity * 2 : 16;
        struct NEMESIS_shadow_entry **tmp = realloc(list->entries,sizeof(struct NEMESIS_shadow_entry *) * new_capacity);
        if (!tmp) { write_log(LOG_ERROR,"erreur de realloc pour entries","NEMESIS_add_shadow_entry()");perror("realloc"); return false;} // allocation échouée
        list->entries = tmp;
        list->capacity = new_capacity;
    }
    list->entries[list->count++] = to_add;
    return true;
}


/*
 * libere la mamoire allouer pour la structure ainsi que pour sont attribut entries pour cela appeller void NEMESIS_free_shadow_entry(struct NEMESIS_shadow_entry *to_free){
 * pour chacune des entrées.
*/
void NEMESIS_free_shadow_entry_list(struct NEMESIS_shadow_entry_list *to_free)
{
    for (int i=0;i<to_free->count;i++) {
        NEMESIS_free_shadow_entry(to_free->entries[i]);
    }
    free(to_free->entries);
    //free(to_free);
}