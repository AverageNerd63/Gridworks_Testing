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
    FILE            *log_file;
    struct { GwLogSink fn; void *user; } sinks[MAX_SINKS];
    u32 sink_count;
} g_logger;

static const char *s_labels[] = { "TRACE","DEBUG","INFO","WARN","ERROR","FATAL" };
static const char *s_colors[] = {
    "\x1b[37m", "\x1b[36m", "\x1b[32m",
    "\x1b[33m", "\x1b[31m", "\x1b[35m"
};

void logger_init(void) {
    g_logger.min_level  = LOG_LEVEL_TRACE;
    g_logger.sink_count = 0;
    InitializeCriticalSection(&g_logger.lock);

    /* enable ANSI on Windows terminal */
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    /* open log file — always, both debug and release */
    CreateDirectoryA("logs", NULL);
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char fname[64];
    strftime(fname, sizeof fname, "logs\\gridworks_%Y%m%d_%H%M%S.log", lt);
    g_logger.log_file = fopen(fname, "w");

    /* keep at most 3 log files — delete oldest when over the limit */
    WIN32_FIND_DATAA fd;
    HANDLE hfind = FindFirstFileA("logs\\gridworks_*.log", &fd);
    if (hfind != INVALID_HANDLE_VALUE) {
        char names[16][MAX_PATH];
        int  count = 0;
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && count < 16) {
                snprintf(names[count], MAX_PATH, "logs\\%s", fd.cFileName);
                count++;
            }
        } while (FindNextFileA(hfind, &fd));
        FindClose(hfind);

        /* filenames are sortable by timestamp — simple insertion sort */
        for (int i = 1; i < count; i++) {
            char tmp[MAX_PATH];
            strncpy(tmp, names[i], MAX_PATH);
            int j = i - 1;
            while (j >= 0 && strcmp(names[j], tmp) > 0) {
                strncpy(names[j + 1], names[j], MAX_PATH);
                j--;
            }
            strncpy(names[j + 1], tmp, MAX_PATH);
        }

        while (count > 3) {
            DeleteFileA(names[0]);
            for (int i = 0; i < count - 1; i++)
                strncpy(names[i], names[i + 1], MAX_PATH);
            count--;
        }
    }
}

void logger_shutdown(void) {
    if (g_logger.log_file) { fclose(g_logger.log_file); g_logger.log_file = NULL; }
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

void logger_log(LogLevel level, LogChannel channel,
                const char *file, int line, const char *fmt, ...) {
    if (level < g_logger.min_level) return;

    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char timebuf[16];
    strftime(timebuf, sizeof timebuf, "%H:%M:%S", lt);

    char msg[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof msg, fmt, args);
    va_end(args);

    char plain[560];
    snprintf(plain, sizeof plain, "[%s] %s", s_labels[level], msg);

    EnterCriticalSection(&g_logger.lock);

    /* terminal: engine logs in debug only; user logs always */
#ifdef NDEBUG
    bool print_terminal = (channel == LOG_CHANNEL_USER);
#else
    bool print_terminal = true;
#endif
    if (print_terminal) {
        fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m %s\n",
                timebuf, s_colors[level], s_labels[level], file, line, msg);
        fflush(stderr);
    }

    /* log file: always write everything */
    if (g_logger.log_file) {
        fprintf(g_logger.log_file, "%s %-5s %s:%d: %s\n",
                timebuf, s_labels[level], file, line, msg);
        fflush(g_logger.log_file);
    }

    /* sinks: pass channel so each sink decides what to show */
    for (u32 i = 0; i < g_logger.sink_count; i++)
        g_logger.sinks[i].fn(level, channel, plain, g_logger.sinks[i].user);

    LeaveCriticalSection(&g_logger.lock);
}

void gw_log_user(int level, const char *msg) {
    logger_log((LogLevel)level, LOG_CHANNEL_USER, "<game>", 0, "%s", msg);
}