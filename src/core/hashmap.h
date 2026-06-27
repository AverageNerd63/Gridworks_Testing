#pragma once
#include "types.h"
#include "str.h"
#include <stdbool.h>

typedef struct HashEntry {
    GwStr  key;
    void  *value;
    bool   occupied;
} HashEntry;

typedef struct HashMap {
    HashEntry *entries;
    usize      cap;
    usize      len;
} HashMap;

void  hashmap_init(HashMap *m, usize initial_cap);
void  hashmap_free(HashMap *m);
void  hashmap_set(HashMap *m, GwStr key, void *value);
void *hashmap_get(HashMap *m, GwStr key);
bool  hashmap_del(HashMap *m, GwStr key);