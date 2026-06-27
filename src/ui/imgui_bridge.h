#pragma once
#include "../core/types.h"
#include <stdbool.h>

bool imgui_init(void *hwnd);
void imgui_shutdown(void);
void imgui_new_frame(void);
void imgui_render(void *cmd_buf);  /* VkCommandBuffer */

void imgui_process_msg(unsigned msg, uintptr_t wparam, intptr_t lparam);