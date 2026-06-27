#include "str.h"
#include <assert.h>

GwStr gwstr_sub(GwStr s, usize start, usize end) {
    assert(start <= end && end <= s.len);
    return (GwStr){ .data = s.data + start, .len = end - start };
}

bool gwstr_starts_with(GwStr s, GwStr prefix) {
    if (prefix.len > s.len) return false;
    return memcmp(s.data, prefix.data, prefix.len) == 0;
}

bool gwstr_ends_with(GwStr s, GwStr suffix) {
    if (suffix.len > s.len) return false;
    return memcmp(s.data + s.len - suffix.len, suffix.data, suffix.len) == 0;
}