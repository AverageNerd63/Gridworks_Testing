#include "gw_host.h"
#include "../core/logger.h"
#include <windows.h>
#include <wchar.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---- hostfxr ABI (subset) --------------------------------------------- */

#define HOSTFXR_CALLTYPE __cdecl
typedef void *hostfxr_handle;

typedef int32_t (HOSTFXR_CALLTYPE *hostfxr_initialize_for_runtime_config_fn)(
    const wchar_t  *runtime_config_path,
    const void     *parameters,
    hostfxr_handle *out_ctx);

typedef int32_t (HOSTFXR_CALLTYPE *hostfxr_get_runtime_delegate_fn)(
    hostfxr_handle  ctx,
    int             type,
    void          **out_delegate);

typedef void (HOSTFXR_CALLTYPE *hostfxr_close_fn)(hostfxr_handle ctx);

#define HDT_LOAD_ASM_AND_GET_FP      5
#define UNMANAGEDCALLERSONLY_METHOD  ((const wchar_t *)-1)

typedef int32_t (*load_asm_get_fn_t)(
    const wchar_t *assembly_path,
    const wchar_t *type_name,
    const wchar_t *method_name,
    const wchar_t *delegate_type_name,
    void          *reserved,
    void         **out_fn);

/* ---- state ------------------------------------------------------------ */

static HMODULE           s_hostfxr = NULL;
static hostfxr_handle    s_ctx     = NULL;
static hostfxr_close_fn  s_close   = NULL;
static load_asm_get_fn_t s_load_fn = NULL;

/* ---- helpers ---------------------------------------------------------- */

static void to_wide(const char *src, wchar_t *dst, int len) {
    MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, len);
}

static void to_abs_wide(const char *src, wchar_t *dst, int len) {
    wchar_t tmp[MAX_PATH];
    to_wide(src, tmp, MAX_PATH);
    GetFullPathNameW(tmp, len, dst, NULL);
}

static bool find_hostfxr(wchar_t *out, int out_len) {
    wchar_t root[MAX_PATH];
    if (!GetEnvironmentVariableW(L"DOTNET_ROOT", root, MAX_PATH)) {
        wchar_t pf[MAX_PATH];
        GetEnvironmentVariableW(L"ProgramFiles", pf, MAX_PATH);
        _snwprintf(root, MAX_PATH, L"%ls\\dotnet", pf);
    }

    wchar_t pattern[MAX_PATH];
    _snwprintf(pattern, MAX_PATH, L"%ls\\host\\fxr\\*", root);

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        LOG_ERROR("[host] hostfxr directory not found: %ls", pattern);
        return false;
    }

    wchar_t best[MAX_PATH] = {0};
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(fd.cFileName, L".") != 0 &&
            wcscmp(fd.cFileName, L"..") != 0)
            wcscpy(best, fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);

    if (!best[0]) { LOG_ERROR("[host] no hostfxr version found"); return false; }
    _snwprintf(out, out_len, L"%ls\\host\\fxr\\%ls\\hostfxr.dll", root, best);
    return true;
}

/* ---- public API ------------------------------------------------------- */

bool gw_host_init(const char *runtime_config_path) {
    wchar_t path[MAX_PATH];
    if (!find_hostfxr(path, MAX_PATH)) return false;

    s_hostfxr = LoadLibraryW(path);
    if (!s_hostfxr) {
        LOG_ERROR("[host] LoadLibrary hostfxr.dll failed (%lu)", GetLastError());
        return false;
    }
    LOG_INFO("[host] loaded %ls", path);

    hostfxr_initialize_for_runtime_config_fn init_fn =
        (hostfxr_initialize_for_runtime_config_fn)
        GetProcAddress(s_hostfxr, "hostfxr_initialize_for_runtime_config");
    hostfxr_get_runtime_delegate_fn get_del =
        (hostfxr_get_runtime_delegate_fn)
        GetProcAddress(s_hostfxr, "hostfxr_get_runtime_delegate");
    s_close = (hostfxr_close_fn)
        GetProcAddress(s_hostfxr, "hostfxr_close");

    if (!init_fn || !get_del || !s_close) {
        LOG_ERROR("[host] missing exports in hostfxr.dll");
        FreeLibrary(s_hostfxr); s_hostfxr = NULL;
        return false;
    }

    wchar_t cfg[MAX_PATH];
    to_abs_wide(runtime_config_path, cfg, MAX_PATH);

    int32_t rc = init_fn(cfg, NULL, &s_ctx);
    if (rc != 0) {
        LOG_ERROR("[host] initialize_for_runtime_config: 0x%x", (unsigned)rc);
        FreeLibrary(s_hostfxr); s_hostfxr = NULL;
        return false;
    }

    rc = get_del(s_ctx, HDT_LOAD_ASM_AND_GET_FP, (void **)&s_load_fn);
    if (rc != 0 || !s_load_fn) {
        LOG_ERROR("[host] get load_assembly_and_get_function_pointer: 0x%x", (unsigned)rc);
        s_close(s_ctx); s_ctx = NULL;
        FreeLibrary(s_hostfxr); s_hostfxr = NULL;
        return false;
    }

    LOG_INFO("[host] CoreCLR runtime ready");
    return true;
}

void gw_host_shutdown(void) {
    if (s_ctx)     { s_close(s_ctx);         s_ctx     = NULL; }
    if (s_hostfxr) { FreeLibrary(s_hostfxr); s_hostfxr = NULL; }
    s_load_fn = NULL;
    LOG_INFO("[host] CoreCLR shut down");
}

static bool shadow_copy_assembly(const wchar_t *src_path, wchar_t *out_path, int out_len) {
    static volatile LONG s_counter = 0;
    LONG idx = InterlockedIncrement(&s_counter);

    wchar_t tmp_dir[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp_dir);

    wchar_t gw_dir[MAX_PATH];
    _snwprintf(gw_dir, MAX_PATH, L"%lsGridworks", tmp_dir);
    CreateDirectoryW(gw_dir, NULL);

    wchar_t shadow_dir[MAX_PATH];
    _snwprintf(shadow_dir, MAX_PATH, L"%ls\\shadow_%ld", gw_dir, (long)idx);
    CreateDirectoryW(shadow_dir, NULL);
    if (GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_ALREADY_EXISTS) {
        LOG_ERROR("[host] shadow dir create failed: %lu", GetLastError());
        return false;
    }

    const wchar_t *fname = wcsrchr(src_path, L'\\');
    fname = fname ? fname + 1 : src_path;

    _snwprintf(out_path, out_len, L"%ls\\%ls", shadow_dir, fname);

    if (!CopyFileW(src_path, out_path, FALSE)) {
        LOG_ERROR("[host] shadow copy failed: %lu", GetLastError());
        return false;
    }
    return true;
}

bool gw_host_get_fn(const char *assembly_path,
                    const char *type_name,
                    const char *method_name,
                    void      **out_fn) {
    if (!s_load_fn) { LOG_ERROR("[host] host not initialized"); return false; }

    wchar_t asm_abs[MAX_PATH];
    to_abs_wide(assembly_path, asm_abs, MAX_PATH);

    /* shadow-copy so the original DLL stays unlocked for hot-rebuild */
    wchar_t asm_shadow[MAX_PATH];
    if (!shadow_copy_assembly(asm_abs, asm_shadow, MAX_PATH)) return false;

    wchar_t type_w[512], method_w[256];
    to_wide(type_name,   type_w,   512);
    to_wide(method_name, method_w, 256);

    int32_t rc = s_load_fn(asm_shadow, type_w, method_w,
                           UNMANAGEDCALLERSONLY_METHOD,
                           NULL, out_fn);
    if (rc != 0) {
        LOG_ERROR("[host] get_fn %s::%s (0x%x)", type_name, method_name, (unsigned)rc);
        return false;
    }
    LOG_INFO("[host] loaded %s::%s from shadow copy", type_name, method_name);
    return true;
}