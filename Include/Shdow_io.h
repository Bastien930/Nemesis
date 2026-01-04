//
// Created by bastien on 10/26/25.
//

#ifndef NEMESIS_SHDOW_IO_H
#define NEMESIS_SHDOW_IO_H

struct NEMESIS_shadow_entry_list {
    struct NEMESIS_shadow_entry **entries;
    int count;
    int capacity;
};

extern struct NEMESIS_shadow_entry_list NEMESIS_shadow_entry_liste;

int NEMESIS_load_shadow_file(const char* path,struct NEMESIS_shadow_entry_list *list);

void NEMESIS_free_shadow_entry_list(struct NEMESIS_shadow_entry_list *to_free);

bool NEMESIS_add_shadow_entry(struct NEMESIS_shadow_entry *to_add,struct NEMESIS_shadow_entry_list *list);

void NEMESIS_init_shadow_entry_list(struct NEMESIS_shadow_entry_list *list) ;

#endif //NEMESIS_SHDOW_IO_H