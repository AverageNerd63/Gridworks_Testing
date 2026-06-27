#pragma once
#include "vk_ctx.h"
#include "vk_swapchain.h"
#include <stdbool.h>

extern VkRenderPass   s_render_pass;
extern VkFramebuffer *s_framebuffers;

bool create_render_pass(void);
void destroy_render_pass(void);
bool create_framebuffers(void);
void destroy_framebuffers(void);