#include "types.h"
#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define MAX_SINKS 8

static struct {
    LogLevel         min_level;
    CRITICAL_SECTION lock;
    struct { GwLogSink fn; void *user; } sinks[MAX_SINKS];
    u32 sink_count;
} g_logger;

static const char *s_labels[] = { "TRACE","DEBUG","INFO","WARN","ERROR","FATAL" };
static const char *s_colors[] = { "\x1b[37m","\x1b[36m","\x1b[32m",
                                   "\x1b[33m","\x1b[31m","\x1b[35m" };

void logger_init(void) {
    g_logger.min_level  = LOG_LEVEL_TRACE;
    g_logger.sink_count = 0;
    InitializeCriticalSection(&g_logger.lock);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void logger_shutdown(void) {
    DeleteCriticalSection(&g_logger.lock);
}

void logger_set_level(LogLevel level) { g_logger.min_level = level; }

void logger_add_sink(GwLogSink sink, void *user) {
    EnterCriticalSection(&g_logger.lock);
    if (g_logger.sink_count < MAX_SINKS) {
        g_logger.sinks[g_logger.sink_count].fn   = sink;
        g_logger.sinks[g_logger.sink_count].user = user;
        g_logger.sink_count++;
    }
    LeaveCriticalSection(&g_logger.lock);
}

void logger_remove_sink(GwLogSink sink) {
    EnterCriticalSection(&g_logger.lock);
    for (u32 i = 0; i < g_logger.sink_count; i++) {
        if (g_logger.sinks[i].fn == sink) {
            g_logger.sinks[i] = g_logger.sinks[--g_logger.sink_count];
            break;
        }
    }
    LeaveCriticalSection(&g_logger.lock);
}

void logger_log(LogLevel level, const char *file, int line, const char *fmt, ...) {
    if (level < g_logger.min_level) return;

    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char timebuf[16];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", lt);

    char plain[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(plain, sizeof(plain), fmt, args);
    va_end(args);

    EnterCriticalSection(&g_logger.lock);

    fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m %s\n",
            timebuf, s_colors[level], s_labels[level], file, line, plain);
    fflush(stderr);

    /* fire sinks with plain (no ANSI) formatted line */
    char sink_line[512];
    snprintf(sink_line, sizeof(sink_line), "[%s] %s", s_labels[level], plain);
    for (u32 i = 0; i < g_logger.sink_count; i++)
        g_logger.sinks[i].fn(level, sink_line, g_logger.sinks[i].user);

    LeaveCriticalSection(&g_logger.lock);
}