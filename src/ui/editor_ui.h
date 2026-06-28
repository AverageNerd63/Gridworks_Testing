#pragma once
#include "../core/types.h"
#include "../core/logger.h"
#include "../platform/input.h"
#include <stdbool.h>

bool editor_ui_init(void);
void editor_ui_shutdown(void);
void editor_ui_build(const InputState *input, f32 dt);
void editor_console_push(LogLevel level, const char *line, void *user);