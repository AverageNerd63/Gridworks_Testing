#include "hashmap.h"
#include <stdlib.h>
#include <assert.h>

static u64 fnv1a(GwStr s) {
    u64 hash = 0xcbf29ce484222325ULL;
    for (usize i = 0; i < s.len; i++) {
        hash ^= (u8)s.data[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

static void hashmap_insert_entry(HashEntry *entries, usize cap, GwStr key, void *value) {
    usize idx = fnv1a(key) & (cap - 1);
    while (entries[idx].occupied && !gwstr_eq(entries[idx].key, key))
        idx = (idx + 1) & (cap - 1);
    entries[idx] = (HashEntry){ .key = key, .value = value, .occupied = true };
}

static void hashmap_grow(HashMap *m) {
    usize      new_cap     = m->cap ? m->cap * 2 : 16;
    HashEntry *new_entries = calloc(new_cap, sizeof(HashEntry));
    for (usize i = 0; i < m->cap; i++)
        if (m->entries[i].occupied)
            hashmap_insert_entry(new_entries, new_cap,
                                 m->entries[i].key, m->entries[i].value);
    free(m->entries);
    m->entries = new_entries;
    m->cap     = new_cap;
}

void hashmap_init(HashMap *m, usize initial_cap) {
    assert(initial_cap && (initial_cap & (initial_cap - 1)) == 0);
    m->entries = calloc(initial_cap, sizeof(HashEntry));
    m->cap     = initial_cap;
    m->len     = 0;
}

void hashmap_free(HashMap *m) {
    free(m->entries);
    m->entries = NULL;
    m->cap = m->len = 0;
}

void hashmap_set(HashMap *m, GwStr key, void *value) {
    if (m->len * 2 >= m->cap) hashmap_grow(m);
    hashmap_insert_entry(m->entries, m->cap, key, value);
    m->len++;
}

void *hashmap_get(HashMap *m, GwStr key) {
    if (!m->cap) return NULL;
    usize idx = fnv1a(key) & (m->cap - 1);
    while (m->entries[idx].occupied) {
        if (gwstr_eq(m->entries[idx].key, key))
            return m->entries[idx].value;
        idx = (idx + 1) & (m->cap - 1);
    }
    return NULL;
}

bool hashmap_del(HashMap *m, GwStr key) {
    if (!m->cap) return false;
    usize idx = fnv1a(key) & (m->cap - 1);
    while (m->entries[idx].occupied) {
        if (gwstr_eq(m->entries[idx].key, key)) {
            m->entries[idx].occupied = false;
            m->len--;
            return true;
        }
        idx = (idx + 1) & (m->cap - 1);
    }
    return false;
}