#pragma once
#include "vk_ctx.h"
#include "../../core/types.h"
#include <stdbool.h>

typedef struct {
    VkImage        color_image;
    VkDeviceMemory color_memory;
    VkImageView    color_view;
    VkSampler      sampler;
    VkRenderPass   render_pass;
    VkFramebuffer  framebuffer;
    void          *imgui_ds;     /* VkDescriptorSet cast — pass to ImGui::Image */
    u32            width, height;
} ScenePass;

extern ScenePass s_scene_pass;

bool scene_pass_create(u32 w, u32 h);
void scene_pass_destroy(void);
bool scene_pass_resize(u32 w, u32 h);
void scene_pass_begin(VkCommandBuffer cmd);
void scene_pass_end(VkCommandBuffer cmd);

void *scene_pass_get_imgui_id(void);

void scene_pass_request_resize(u32 w, u32 h);
bool scene_pass_flush_resize(void);