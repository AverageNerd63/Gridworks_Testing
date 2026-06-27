#pragma once
#include "../core/types.h"
#include <stdbool.h>

typedef enum { FW_CREATED, FW_DELETED, FW_MODIFIED, FW_RENAMED } FwEventType;

typedef struct {
    FwEventType type;
    char        path[512];
} FwEvent;

typedef void (*FwCallback)(FwEvent ev, void *user);

typedef struct FwNative FwNative;  /* opaque, defined per platform */

typedef struct {
    char       watch_path[512];
    FwCallback callback;
    void      *user;
    FwNative  *native;
    bool       running;
} FileWatcher;

bool fw_start(FileWatcher *fw, const char *path, FwCallback cb, void *user);
void fw_stop(FileWatcher *fw);