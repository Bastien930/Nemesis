//
// Created by bastien on 10/24/25.
//
#include "../Include/Shadow.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *strdup_safe(const char *s) {
    if (!s) return NULL;             // protection contre NULL
    size_t n = strlen(s) + 1;       // +1 pour '\0'
    char *p = malloc(n);             // allocation
    if (!p) return NULL;             // malloc failed
    memcpy(p, s, n);                 // copie mémoire complète (y compris '\0')
    return p;
}


/*
 * a chaque fois que tu voudra copier utilise strdup_safe pour just allouer de la mémoire et copier les donnes dans le nouvelle espace memoire.
 * param const char *line : line du fichier shadow
 * utilise strtok pour separer les lignes au $
 * renvoie l'adresse memoire de la structure rempli.
 * ps : le type d'algo est un enum dans le fichier Shadow.h
*/
struct da_shadow_entry *da_parse_shadow_line(const char *line){
    ;
}

/*
 * libere les champs de la strcuture passer en parametre.
 * libere la structure passer en parametre.
 * return : void.
*/
void da_free_shadow_entry(struct da_shadow_entry *to_free){
    ;
}