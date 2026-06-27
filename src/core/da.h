#pragma once
#include "types.h"
/*
 * Embed this at the top of any struct you want to use as a dynamic array:
 *
 *   typedef struct { DA_HEADER(MyItem); } MyItemArray;
 *
 * Or use the typed shorthand:
 *
 *   DA(f32) floats = {0};
 *   da_push(&floats, 3.14f);
 */

#define DA_HEADER(T) T *data; usize len; usize cap

#define DA(T) struct { DA_HEADER(T); }

#define da_push(a, val)                                         \
    do {                                                        \
        if ((a)->len >= (a)->cap) {                             \
            (a)->cap  = (a)->cap ? (a)->cap * 2 : 8;           \
            (a)->data = realloc((a)->data,                      \
                                (a)->cap * sizeof(*(a)->data)); \
        }                                                       \
        (a)->data[(a)->len++] = (val);                          \
    } while (0)

#define da_pop(a)     ((a)->data[--(a)->len])
#define da_last(a)    ((a)->data[(a)->len - 1])
#define da_clear(a)   ((a)->len = 0)
#define da_free(a)    (free((a)->data), (a)->data = NULL, (a)->len = 0, (a)->cap = 0)