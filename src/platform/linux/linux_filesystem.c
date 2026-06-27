#include "../filesystem.h"
#include "../../core/logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

bool fs_read_file(const char *path, FileData *out) {
    FILE *f = fopen(path, "rb");
    if (!f) { LOG_ERROR("fs_read_file: '%s'", path); return false; }
    fseek(f, 0, SEEK_END); out->size = (usize)ftell(f); fseek(f, 0, SEEK_SET);
    out->data = malloc(out->size + 1);
    fread(out->data, 1, out->size, f);
    out->data[out->size] = '\0';
    fclose(f);
    return true;
}

void fs_free_file(FileData *f) { free(f->data); f->data = NULL; f->size = 0; }

bool fs_write_file(const char *path, const u8 *data, usize size) {
    FILE *f = fopen(path, "wb");
    if (!f) { LOG_ERROR("fs_write_file: '%s'", path); return false; }
    usize written = fwrite(data, 1, size, f);
    fclose(f);
    return written == size;
}

bool fs_exists(const char *path)   { struct stat s; return stat(path, &s) == 0; }
bool fs_make_dir(const char *path) { return mkdir(path, 0755) == 0 || errno == EEXIST; }

GwStr fs_filename(GwStr path) {
    for (usize i = path.len; i-- > 0;)
        if (path.data[i] == '/') return gwstr_sub(path, i + 1, path.len);
    return path;
}
GwStr fs_extension(GwStr path) {
    GwStr name = fs_filename(path);
    for (usize i = name.len; i-- > 0;)
        if (name.data[i] == '.') return gwstr_sub(name, i, name.len);
    return (GwStr){0};
}
GwStr fs_directory(GwStr path) {
    for (usize i = path.len; i-- > 0;)
        if (path.data[i] == '/') return gwstr_sub(path, 0, i);
    return (GwStr){0};
}