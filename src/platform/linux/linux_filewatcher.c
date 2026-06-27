#include "../filewatcher.h"
#include "../../core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>

struct FwNative { pthread_t thread; };

static void *fw_thread(void *arg) {
    FileWatcher *fw = (FileWatcher *)arg;
    int fd = inotify_init();
    inotify_add_watch(fd, fw->watch_path,
        IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO);

    char buf[4096];
    while (fw->running) {
        int len = (int)read(fd, buf, sizeof(buf));
        if (len <= 0) continue;
        struct inotify_event *ev = (struct inotify_event *)buf;
        while ((char *)ev < buf + len) {
            FwEvent gw_ev = {0};
            if (ev->mask & IN_CREATE)            gw_ev.type = FW_CREATED;
            else if (ev->mask & IN_DELETE)       gw_ev.type = FW_DELETED;
            else if (ev->mask & IN_MODIFY)       gw_ev.type = FW_MODIFIED;
            else if (ev->mask & IN_MOVED_TO)     gw_ev.type = FW_RENAMED;
            else { ev = (struct inotify_event *)((char *)ev + sizeof(*ev) + ev->len); continue; }
            strncpy(gw_ev.path, ev->name, sizeof(gw_ev.path) - 1);
            fw->callback(gw_ev, fw->user);
            ev = (struct inotify_event *)((char *)ev + sizeof(*ev) + ev->len);
        }
    }
    close(fd);
    return NULL;
}

bool fw_start(FileWatcher *fw, const char *path, FwCallback cb, void *user) {
    strncpy(fw->watch_path, path, sizeof(fw->watch_path) - 1);
    fw->callback = cb;
    fw->user     = user;
    fw->running  = true;
    fw->native   = malloc(sizeof(FwNative));
    return pthread_create(&fw->native->thread, NULL, fw_thread, fw) == 0;
}

void fw_stop(FileWatcher *fw) {
    fw->running = false;
    pthread_join(fw->native->thread, NULL);
    free(fw->native);
    fw->native = NULL;
}