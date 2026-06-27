#pragma once
#include "vk_ctx.h"
#include <stdbool.h>

#define MAX_SWAPCHAIN_IMAGES 8

typedef struct {
    VkSwapchainKHR swapchain;
    VkFormat       image_format;
    VkExtent2D     extent;
    VkImage       *images;
    VkImageView   *image_views;
    u32            image_count;
} Swapchain;

extern Swapchain s_swapchain;

bool create_swapchain(u32 w, u32 h);
void destroy_swapchain(void);