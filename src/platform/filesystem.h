#pragma once
#include "../core/types.h"
#include "../core/str.h"
#include <stdbool.h>

typedef struct {
    u8   *data;
    usize size;
} FileData;

bool  fs_read_file(const char *path, FileData *out);
void  fs_free_file(FileData *f);
bool  fs_write_file(const char *path, const u8 *data, usize size);
bool  fs_exists(const char *path);
bool  fs_make_dir(const char *path);
GwStr fs_filename(GwStr path);
GwStr fs_extension(GwStr path);
GwStr fs_directory(GwStr path);