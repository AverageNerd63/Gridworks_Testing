#pragma once
#include "vk_loader.h"
#include "../../core/types.h"

typedef struct {
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR             surface;
    VkPhysicalDevice         gpu;
    VkDevice                 device;
    VkQueue                  graphics_queue;
    VkQueue                  present_queue;
    u32                      graphics_family;
    u32                      present_family;
    VkDescriptorPool         imgui_pool;
} VkCtx;

extern VkCtx s_ctx;

#define VK_CHECK(call) \
    do { \
        VkResult _r = (call); \
        if (_r != VK_SUCCESS) { \
            LOG_ERROR("[vulkan] %s failed: %d", #call, _r); \
            return false; \
        } \
    } while (0)