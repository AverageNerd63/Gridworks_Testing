#pragma once
#include "types.h"
#include <string.h>
#include <stdbool.h>

typedef struct {
    char  *data;
    usize  len;
} GwStr;

#define GW_STR(lit)       ((GwStr){ .data = (char *)(lit), .len = sizeof(lit) - 1 })
#define GW_STR_FMT        "%.*s"
#define GW_STR_ARG(s)     (int)(s).len, (s).data

static inline GwStr gwstr_from_cstr(const char *s) {
    return (GwStr){ .data = (char *)s, .len = strlen(s) };
}

static inline bool gwstr_eq(GwStr a, GwStr b) {
    return a.len == b.len && memcmp(a.data, b.data, a.len) == 0;
}

static inline bool gwstr_empty(GwStr s) {
    return s.len == 0 || !s.data;
}

GwStr gwstr_sub(GwStr s, usize start, usize end);
bool  gwstr_starts_with(GwStr s, GwStr prefix);
bool  gwstr_ends_with(GwStr s, GwStr suffix);