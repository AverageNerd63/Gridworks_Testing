#pragma once

typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
} LogLevel;

typedef enum {
    LOG_CHANNEL_ENGINE = 0,  /* internal engine logs — hidden from editor console */
    LOG_CHANNEL_USER,        /* game/user logs — shown in editor console          */
} LogChannel;

void logger_init(void);
void logger_shutdown(void);
void logger_set_level(LogLevel level);
void logger_log(LogLevel level, LogChannel channel,
                const char *file, int line, const char *fmt, ...);

/* sink receives channel so it can filter */
typedef void (*GwLogSink)(LogLevel level, LogChannel channel,
                          const char *line, void *user);
void logger_add_sink(GwLogSink sink, void *user);
void logger_remove_sink(GwLogSink sink);

void gw_log_user(int level, const char *msg);

/* engine-internal logs (hidden from editor console) */
#define LOG_TRACE(...) logger_log(LOG_LEVEL_TRACE, LOG_CHANNEL_ENGINE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, LOG_CHANNEL_ENGINE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  logger_log(LOG_LEVEL_INFO,  LOG_CHANNEL_ENGINE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  logger_log(LOG_LEVEL_WARN,  LOG_CHANNEL_ENGINE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOG_LEVEL_ERROR, LOG_CHANNEL_ENGINE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) logger_log(LOG_LEVEL_FATAL, LOG_CHANNEL_ENGINE, __FILE__, __LINE__, __VA_ARGS__)

/* user/game logs (visible in editor console) */
#define LOG_USER_TRACE(...) logger_log(LOG_LEVEL_TRACE, LOG_CHANNEL_USER, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_USER_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, LOG_CHANNEL_USER, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_USER_INFO(...)  logger_log(LOG_LEVEL_INFO,  LOG_CHANNEL_USER, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_USER_WARN(...)  logger_log(LOG_LEVEL_WARN,  LOG_CHANNEL_USER, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_USER_ERROR(...) logger_log(LOG_LEVEL_ERROR, LOG_CHANNEL_USER, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_USER_FATAL(...) logger_log(LOG_LEVEL_FATAL, LOG_CHANNEL_USER, __FILE__, __LINE__, __VA_ARGS__)