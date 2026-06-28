#pragma once
#include "../core/types.h"
#include <stdbool.h>

bool imgui_init(void *hwnd);
void imgui_shutdown(void);
void imgui_new_frame(void);
void imgui_render(void *cmd_buf);  /* VkCommandBuffer */

void imgui_process_msg(unsigned msg, uintptr_t wparam, intptr_t lparam);

/* Register a Vulkan image as an ImGui texture. Returns ImTextureID (cast to void*). */
void *imgui_register_texture(void *sampler, void *image_view);
void  imgui_unregister_texture(void *imgui_ds);