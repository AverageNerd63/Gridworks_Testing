#include "../filewatcher.h"
#include "../../core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>

struct FwNative { HANDLE thread; };

static DWORD WINAPI fw_thread(LPVOID arg) {
    FileWatcher *fw = (FileWatcher *)arg;

    HANDLE dir = CreateFileA(
        fw->watch_path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    if (dir == INVALID_HANDLE_VALUE) {
        LOG_ERROR("filewatcher: cannot open '%s'", fw->watch_path);
        return 1;
    }

    u8         buf[4096];
    OVERLAPPED ov = {0};
    ov.hEvent     = CreateEventA(NULL, FALSE, FALSE, NULL);

    while (fw->running) {
        ReadDirectoryChangesW(dir, buf, sizeof(buf), TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
            NULL, &ov, NULL);

        if (WaitForSingleObject(ov.hEvent, 100) != WAIT_OBJECT_0) continue;

        DWORD bytes;
        GetOverlappedResult(dir, &ov, &bytes, FALSE);

        FILE_NOTIFY_INFORMATION *info = (FILE_NOTIFY_INFORMATION *)buf;
        for (;;) {
            FwEvent ev = {0};
            switch (info->Action) {
                case FILE_ACTION_ADDED:            ev.type = FW_CREATED;  break;
                case FILE_ACTION_REMOVED:          ev.type = FW_DELETED;  break;
                case FILE_ACTION_MODIFIED:         ev.type = FW_MODIFIED; break;
                case FILE_ACTION_RENAMED_NEW_NAME: ev.type = FW_RENAMED;  break;
                default: goto next;
            }
            WideCharToMultiByte(CP_UTF8, 0,
                info->FileName, (i32)(info->FileNameLength / sizeof(WCHAR)),
                ev.path, (int)sizeof(ev.path) - 1, NULL, NULL);
            fw->callback(ev, fw->user);
            next:
            if (!info->NextEntryOffset) break;
            info = (FILE_NOTIFY_INFORMATION *)((u8 *)info + info->NextEntryOffset);
        }
    }

    CloseHandle(ov.hEvent);
    CloseHandle(dir);
    return 0;
}

bool fw_start(FileWatcher *fw, const char *path, FwCallback cb, void *user) {
    strncpy(fw->watch_path, path, sizeof(fw->watch_path) - 1);
    fw->callback = cb;
    fw->user     = user;
    fw->running  = true;
    fw->native   = malloc(sizeof(FwNative));
    fw->native->thread = CreateThread(NULL, 0, fw_thread, fw, 0, NULL);
    return fw->native->thread != NULL;
}

void fw_stop(FileWatcher *fw) {
    fw->running = false;
    WaitForSingleObject(fw->native->thread, INFINITE);
    CloseHandle(fw->native->thread);
    free(fw->native);
    fw->native = NULL;
}