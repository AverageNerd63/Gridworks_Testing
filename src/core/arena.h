#pragma once
#include "types.h"

typedef struct Arena {
    u8 *base;
    usize size;
    usize offset;
} Arena;

void arena_init(Arena *a, void *backing, usize size);
void *arena_alloc(Arena *a, usize size, usize align);
void arena_reset(Arena *a);

#define arena_push(a, T)      ((T *)arena_alloc((a), sizeof(T),       _Alignof(T)))
#define arena_push_n(a, T, n) ((T *)arena_alloc((a), sizeof(T) * (n), _Alignof(T)))