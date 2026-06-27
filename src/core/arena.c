#include "arena.h"
#include <string.h>

static usize align_foward(usize offset, usize align) {
    usize mod = offset & (align - 1);
    return mod ? offset + (align - mod) : offset;
}

void arena_init(Arena *a, void *backing, usize size) {
    a->base   = (u8 *)backing;
    a->size   = size;
    a->offset = 0;
}

void *arena_alloc(Arena *a, usize size, usize align) {
    usize aligned = align_foward(a->offset, align);
    if (aligned + size > a->size) return NULL;
    void *ptr = a->base + aligned;
    a->offset = aligned + size;
    memset(ptr, 0, size);
    return ptr;
}

void arena_reset(Arena *a) {
    a->offset = 0;
}