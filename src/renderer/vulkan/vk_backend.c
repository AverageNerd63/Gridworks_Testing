#include "vk_backend.h"
#include "vk_loader.h"
#include "vk_ctx.h"
#include "vk_swapchain.h"
#include "vk_renderpass.h"
#include "vk_scene_pass.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"
#include "vk_frame.h"
#include "../../ui/imgui_bridge.h"
#include "../../core/logger.h"
#include "../../platform/vk_surface.h"
#include "../../platform/window.h"
#include <stdlib.h>
#include <string.h>

VkCtx s_ctx;

/* ---- instance --------------------------------------------------------- */

static bool create_instance(void) {
    static const char *exts[3];
    u32 ext_count = 2;
    exts[0] = VK_KHR_SURFACE_EXTENSION_NAME;
    exts[1] = platform_vk_surface_ext();
#ifndef NDEBUG
    exts[2]   = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    ext_count = 3;
#endif

    /* check validation layer availability */
    const char *layers[]   = { "VK_LAYER_KHRONOS_validation" };
    u32         layer_count = 0;
#ifndef NDEBUG
    u32 avail = 0;
    vk.vkEnumerateInstanceLayerProperties(&avail, NULL);
    VkLayerProperties *lp = malloc(avail * sizeof(VkLayerProperties));
    vk.vkEnumerateInstanceLayerProperties(&avail, lp);
    for (u32 i = 0; i < avail; i++)
        if (__builtin_strcmp(lp[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
            { layer_count = 1; break; }
    free(lp);
#endif

    VkApplicationInfo app = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "Gridworks",
        .applicationVersion = VK_MAKE_VERSION(0,1,0),
        .pEngineName        = "Gridworks Engine",
        .engineVersion      = VK_MAKE_VERSION(0,1,0),
        .apiVersion         = VK_API_VERSION_1_3,
    };
    VkInstanceCreateInfo ci = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &app,
        .enabledExtensionCount   = ext_count,
        .ppEnabledExtensionNames = exts,
        .enabledLayerCount       = layer_count,
        .ppEnabledLayerNames     = layer_count ? layers : NULL,
    };
    VK_CHECK(vk.vkCreateInstance(&ci, NULL, &s_ctx.instance));
    vk_loader_load_instance_funcs(s_ctx.instance);

#ifndef NDEBUG
    if (vk.vkCreateDebugUtilsMessengerEXT) {
        extern VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
            const VkDebugUtilsMessengerCallbackDataEXT *, void *);
        VkDebugUtilsMessengerCreateInfoEXT dm = {
            .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debug_callback,
        };
        vk.vkCreateDebugUtilsMessengerEXT(s_ctx.instance, &dm, NULL, &s_ctx.debug_messenger);
    }
#endif

    LOG_INFO("[vulkan] instance created");
    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT *data,
    void                                       *user)
{
    (void)type; (void)user;
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        LOG_ERROR("[VK] %s", data->pMessage);
    else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        LOG_WARN("[VK] %s", data->pMessage);
    else
        LOG_DEBUG("[VK] %s", data->pMessage);
    return VK_FALSE;
}

/* ---- physical device + queues ----------------------------------------- */

static bool pick_gpu(void) {
    u32 count = 0;
    vk.vkEnumeratePhysicalDevices(s_ctx.instance, &count, NULL);
    if (!count) { LOG_ERROR("[vulkan] no GPUs"); return false; }
    VkPhysicalDevice *gpus = malloc(count * sizeof(VkPhysicalDevice));
    vk.vkEnumeratePhysicalDevices(s_ctx.instance, &count, gpus);
    s_ctx.gpu = gpus[0];
    for (u32 i = 0; i < count; i++) {
        VkPhysicalDeviceProperties p;
        vk.vkGetPhysicalDeviceProperties(gpus[i], &p);
        if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { s_ctx.gpu = gpus[i]; break; }
    }
    VkPhysicalDeviceProperties chosen;
    vk.vkGetPhysicalDeviceProperties(s_ctx.gpu, &chosen);
    LOG_INFO("[vulkan] GPU: %s", chosen.deviceName);
    free(gpus);
    return true;
}

static bool find_queue_families(void) {
    u32 count = 0;
    vk.vkGetPhysicalDeviceQueueFamilyProperties(s_ctx.gpu, &count, NULL);
    VkQueueFamilyProperties *props = malloc(count * sizeof(VkQueueFamilyProperties));
    vk.vkGetPhysicalDeviceQueueFamilyProperties(s_ctx.gpu, &count, props);
    s_ctx.graphics_family = s_ctx.present_family = UINT32_MAX;
    for (u32 i = 0; i < count; i++) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) s_ctx.graphics_family = i;
        VkBool32 present = VK_FALSE;
        vk.vkGetPhysicalDeviceSurfaceSupportKHR(s_ctx.gpu, i, s_ctx.surface, &present);
        if (present) s_ctx.present_family = i;
        if (s_ctx.graphics_family != UINT32_MAX && s_ctx.present_family != UINT32_MAX) break;
    }
    free(props);
    if (s_ctx.graphics_family == UINT32_MAX) { LOG_ERROR("[vulkan] no graphics queue"); return false; }
    if (s_ctx.present_family  == UINT32_MAX) { LOG_ERROR("[vulkan] no present queue");  return false; }
    LOG_INFO("[vulkan] queues — graphics: %u  present: %u", s_ctx.graphics_family, s_ctx.present_family);
    return true;
}

static bool create_device(void) {
    float priority = 1.0f;
    u32 families[2] = { s_ctx.graphics_family, s_ctx.present_family };
    u32 unique = (s_ctx.graphics_family == s_ctx.present_family) ? 1 : 2;
    VkDeviceQueueCreateInfo qcis[2] = {0};
    for (u32 i = 0; i < unique; i++) {
        qcis[i].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qcis[i].queueFamilyIndex = families[i];
        qcis[i].queueCount       = 1;
        qcis[i].pQueuePriorities = &priority;
    }
    static const char *dev_exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkPhysicalDeviceFeatures features = {0};
    VkDeviceCreateInfo ci = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount    = unique,
        .pQueueCreateInfos       = qcis,
        .enabledExtensionCount   = 1,
        .ppEnabledExtensionNames = dev_exts,
        .pEnabledFeatures        = &features,
    };
    VK_CHECK(vk.vkCreateDevice(s_ctx.gpu, &ci, NULL, &s_ctx.device));
    vk_loader_load_device_funcs(s_ctx.device);
    vk.vkGetDeviceQueue(s_ctx.device, s_ctx.graphics_family, 0, &s_ctx.graphics_queue);
    vk.vkGetDeviceQueue(s_ctx.device, s_ctx.present_family,  0, &s_ctx.present_queue);
    LOG_INFO("[vulkan] logical device created");
    return true;
}

/* ---- backend API ------------------------------------------------------ */

static bool vk_init(const GwRendererDesc *desc) {
    if (!vk_loader_init())             return false;
    if (!create_instance())            return false;
    if (!platform_vk_create_surface(s_ctx.instance, vk.vkGetInstanceProcAddr,
                                     desc->window, &s_ctx.surface)) return false;
    if (!pick_gpu())                   return false;
    if (!find_queue_families())        return false;
    if (!create_device())              return false;
    if (!create_swapchain((u32)desc->window->width, (u32)desc->window->height)) return false;
    if (!create_render_pass())         return false;
    if (!create_framebuffers())        return false;

    /* imgui pool + init BEFORE scene_pass_create (AddTexture needs initialized backend) */
    {
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER,                16 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          16 },
        };
        VkDescriptorPoolCreateInfo pool_ci = {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets       = 16,
            .poolSizeCount = 3, .pPoolSizes = pool_sizes,
        };
        VK_CHECK(vk.vkCreateDescriptorPool(s_ctx.device, &pool_ci, NULL, &s_ctx.imgui_pool));
    }
    if (!imgui_init(gw_window_get_hwnd(desc->window))) return false;

    /* scene pass after imgui so AddTexture is safe; pipeline after scene pass for render pass ref */
    if (!scene_pass_create((u32)desc->window->width, (u32)desc->window->height)) return false;
    if (!create_pipeline())            return false;
    if (!create_mesh_buffers())        return false;
    if (!create_frames())              return false;

    LOG_INFO("[vulkan] backend ready");
    return true;
}

static void vk_shutdown(void) {
    if (s_ctx.device) vk.vkDeviceWaitIdle(s_ctx.device);
    destroy_frames();
    destroy_mesh_buffers();
    destroy_pipeline();
    scene_pass_destroy();          /* unregisters texture — must be before imgui_shutdown */
    imgui_shutdown();              /* destroys ImGui Vulkan backend */
    if (s_ctx.imgui_pool)
        vk.vkDestroyDescriptorPool(s_ctx.device, s_ctx.imgui_pool, NULL);
    destroy_framebuffers();
    destroy_render_pass();
    destroy_swapchain();
    if (s_ctx.device)         vk.vkDestroyDevice(s_ctx.device, NULL);
    if (s_ctx.surface)        vk.vkDestroySurfaceKHR(s_ctx.instance, s_ctx.surface, NULL);
    if (s_ctx.debug_messenger && vk.vkDestroyDebugUtilsMessengerEXT)
        vk.vkDestroyDebugUtilsMessengerEXT(s_ctx.instance, s_ctx.debug_messenger, NULL);
    if (s_ctx.instance)       vk.vkDestroyInstance(s_ctx.instance, NULL);
    vk_loader_shutdown();
    memset(&s_ctx, 0, sizeof(s_ctx));
    LOG_INFO("[vulkan] shutdown complete");
}

static void vk_wait_idle(void) {
    if (s_ctx.device) vk.vkDeviceWaitIdle(s_ctx.device);
}

static void vk_on_resize(u32 w, u32 h) {
    if (s_ctx.device) vk.vkDeviceWaitIdle(s_ctx.device);
    destroy_frames();
    destroy_framebuffers();
    destroy_swapchain();
    create_swapchain(w, h);
    create_framebuffers();
    scene_pass_resize(w, h);
    create_frames();
    LOG_INFO("[vulkan] resized %ux%u", w, h);
}

const GwRendererAPI gw_renderer_vulkan = {
    .init        = vk_init,
    .shutdown    = vk_shutdown,
    .begin_frame = vk_begin_frame,
    .end_frame   = vk_end_frame,
    .wait_idle   = vk_wait_idle,
    .on_resize   = vk_on_resize,
};