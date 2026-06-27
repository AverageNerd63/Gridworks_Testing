#pragma once

typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
} LogLevel;

void logger_init(void);
void logger_shutdown(void);
void logger_set_level(LogLevel level);
void logger_log(LogLevel level, const char *file, int line, const char *fmt, ...);

/* Sink callback: receives the fully-formatted line (no ANSI codes). */
typedef void (*GwLogSink)(LogLevel level, const char *line, void *user);
void logger_add_sink(GwLogSink sink, void *user);
void logger_remove_sink(GwLogSink sink);

#define LOG_TRACE(...) logger_log(LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  logger_log(LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  logger_log(LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) logger_log(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)