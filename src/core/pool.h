#pragma once
#include "types.h"

typedef struct PoolBlock {
    struct PoolBlock *next;
} PoolBlock;

typedef struct Pool {
    u8        *base;
    usize     block_size;
    usize     block_count;
    PoolBlock *free_list;
} Pool;

void pool_init(Pool *p, void *backing, usize block_size, usize block_count);
void *pool_alloc(Pool *p);
void pool_free(Pool *p, void *ptr);