#pragma once
#include "window.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>

const char *platform_vk_surface_ext(void);
bool        platform_vk_create_surface(VkInstance               instance,
                                        PFN_vkGetInstanceProcAddr get_proc,
                                        const GwWindow           *window,
                                        VkSurfaceKHR             *out);