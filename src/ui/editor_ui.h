#pragma once
#include "../core/types.h"
#include "../core/logger.h"
#include "../platform/input.h"
#include <stdbool.h>

typedef enum {
    PLAY_STATE_STOPPED = 0,
    PLAY_STATE_PLAYING,
    PLAY_STATE_PAUSED,
} PlayState;

PlayState editor_ui_get_play_state(void);
void      editor_ui_set_play_state(PlayState state);

bool editor_ui_init(void);
void editor_ui_shutdown(void);
void editor_ui_build(const InputState *input, f32 dt);
void editor_console_push(LogLevel level, LogChannel channel, const char *line, void *user);