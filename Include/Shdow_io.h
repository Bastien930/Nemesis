//
// Created by bastien on 10/26/25.
//

#ifndef DA_SHDOW_IO_H
#define DA_SHDOW_IO_H

struct da_shadow_entry_list {
    struct da_shadow_entry **entries;
    int count;
    int capacity;
};

extern struct da_shadow_entry_list da_shadow_entry_liste;

int da_load_shadow_file(const char* path,struct da_shadow_entry_list *list);

void da_free_shadow_entry_list(struct da_shadow_entry_list *to_free);

bool da_add_shadow_entry(struct da_shadow_entry *to_add,struct da_shadow_entry_list *list);

void da_init_shadow_entry_list(struct da_shadow_entry_list *list) ;

#endif //DA_SHDOW_IO_H