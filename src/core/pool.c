#include "types.h"
#include "pool.h"
#include <assert.h>
#include <string.h>

void pool_init(Pool *p, void *backing, usize block_size, usize block_count) {
    assert(block_size >= sizeof(PoolBlock));
    p->base        = (u8 *)backing;
    p->block_size  = block_size;
    p->block_count = block_count;
    p->free_list   = NULL;
}

void *pool_alloc(Pool *p) {
    if (!p->free_list) return NULL;
    PoolBlock *block = p->free_list;
    p->free_list = block->next;
    memset(block, 0, p->block_size);
    return block;
}

void pool_free(Pool *p, void *ptr) {
    PoolBlock *block = (PoolBlock *)ptr;
    block->next = p->free_list;
    p->free_list = block;
}