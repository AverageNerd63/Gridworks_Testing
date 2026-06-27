#pragma once
#include "vk_ctx.h"
#include "../../core/types.h"
#include <stdbool.h>

typedef struct {
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;
    VkSampler      sampler;
    VkDescriptorSetLayout set_layout;
    VkDescriptorPool      pool;
    VkDescriptorSet       set;
    u32            width, height;
} GwTexture;

/* Upload an R8 (single channel) bitmap to a GPU texture with a descriptor set. */
bool gw_texture_create_r8(GwTexture *tex, const u8 *pixels, u32 w, u32 h,
                           VkCommandPool cmd_pool);
void gw_texture_destroy(GwTexture *tex);