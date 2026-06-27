#include "../filesystem.h"
#include "../../core/logger.h"
#include <stdlib.h>
#include <windows.h>

bool fs_read_file(const char *path, FileData *out) {
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) { LOG_ERROR("fs_read_file: '%s'", path); return false; }
    LARGE_INTEGER sz;
    GetFileSizeEx(h, &sz);
    out->size = (usize)sz.QuadPart;
    out->data = malloc(out->size + 1);
    DWORD read;
    ReadFile(h, out->data, (DWORD)out->size, &read, NULL);
    out->data[out->size] = '\0';
    CloseHandle(h);
    return true;
}

void fs_free_file(FileData *f) { free(f->data); f->data = NULL; f->size = 0; }

bool fs_write_file(const char *path, const u8 *data, usize size) {
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0,
                           NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) { LOG_ERROR("fs_write_file: '%s'", path); return false; }
    DWORD written;
    WriteFile(h, data, (DWORD)size, &written, NULL);
    CloseHandle(h);
    return (usize)written == size;
}

bool fs_exists(const char *path)  { return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES; }
bool fs_make_dir(const char *path){ return CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS; }

GwStr fs_filename(GwStr path) {
    for (usize i = path.len; i-- > 0;)
        if (path.data[i] == '/' || path.data[i] == '\\')
            return gwstr_sub(path, i + 1, path.len);
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
        if (path.data[i] == '/' || path.data[i] == '\\')
            return gwstr_sub(path, 0, i);
    return (GwStr){0};
}