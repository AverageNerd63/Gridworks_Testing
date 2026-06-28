#pragma once
#include "vk_ctx.h"
#include "vk_swapchain.h"
#include "vk_renderpass.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"
#include <stdbool.h>

#define FRAMES_IN_FLIGHT 2

typedef struct {
    VkCommandPool   cmd_pool;
    VkCommandBuffer cmd_buf;
    VkFence         in_flight;
    VkSemaphore     _acquire_sem;
} Frame;

extern Frame      s_frames[FRAMES_IN_FLIGHT];
extern u32        s_frame_idx;
extern u32        s_image_idx;
extern VkSemaphore s_acquire_sems[FRAMES_IN_FLIGHT];
extern VkSemaphore s_render_finished[MAX_SWAPCHAIN_IMAGES];

bool create_frames(void);
void destroy_frames(void);
bool vk_begin_frame(void);
void vk_end_frame(void);

void vk_set_camera_vp(const f32 *mat16);