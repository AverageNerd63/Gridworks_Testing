#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <windows.h>

static struct {
    LogLevel         min_level;
    CRITICAL_SECTION lock;
} g_logger;

static const char *s_labels[] = { "TRACE", "DEBUG", "INFO",  "WARN",  "ERROR", "FATAL" };
static const char *s_colors[] = { "\x1b[37m", "\x1b[36m", "\x1b[32m",
                                   "\x1b[33m", "\x1b[31m", "\x1b[35m" };

void logger_init(void) {
    g_logger.min_level = LOG_LEVEL_TRACE;
    InitializeCriticalSection(&g_logger.lock);

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void logger_shutdown(void) {
    DeleteCriticalSection(&g_logger.lock);
}

void logger_set_level(LogLevel level) {
    g_logger.min_level = level;
}

void logger_log(LogLevel level, const char *file, int line, const char *fmt, ...) {
    if (level < g_logger.min_level) return;

    time_t t  = time(NULL);
    struct tm *lt = localtime(&t);
    char timebuf[16];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", lt);

    EnterCriticalSection(&g_logger.lock);

    fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
            timebuf, s_colors[level], s_labels[level], file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);
    fflush(stderr);

    LeaveCriticalSection(&g_logger.lock);
}