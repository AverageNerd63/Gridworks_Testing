#include "../process.h"
#include "../../core/logger.h"
#include <windows.h>
#include <string.h>

void gw_process_spawn(const char *cmd) {
    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    char buf[1024];
    strncpy(buf, cmd, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    if (!CreateProcessA(NULL, buf, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        LOG_ERROR("[process] spawn failed: %s (%lu)", cmd, GetLastError());
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}