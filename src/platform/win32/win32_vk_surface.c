#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "../vk_surface.h"
#include "../window.h"
#include "../../core/logger.h"

const char *platform_vk_surface_ext(void) {
    return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

bool platform_vk_create_surface(VkInstance instance, PFN_vkGetInstanceProcAddr get_proc,
                                 const GwWindow *window, VkSurfaceKHR *out) {
    PFN_vkCreateWin32SurfaceKHR fn =
        (PFN_vkCreateWin32SurfaceKHR)get_proc(instance, "vkCreateWin32SurfaceKHR");
    if (!fn) { LOG_ERROR("[vulkan] vkCreateWin32SurfaceKHR not found"); return false; }

    VkWin32SurfaceCreateInfoKHR ci = {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandleA(NULL),
        .hwnd      = (HWND)gw_window_get_hwnd(window),
    };
    return fn(instance, &ci, NULL, out) == VK_SUCCESS;
}