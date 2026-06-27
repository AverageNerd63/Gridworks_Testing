#include "vk_loader.h"
#include "../../core/logger.h"
#include <windows.h>
#include <string.h>

VkFuncTable vk;

static HMODULE s_lib;

#define LOAD_GLOBAL(fn) \
    vk.fn = (PFN_##fn)vk.vkGetInstanceProcAddr(NULL, #fn); \
    if (!vk.fn) { LOG_ERROR("vk_loader: failed to load " #fn); return false; }

#define LOAD_INSTANCE(fn) \
    vk.fn = (PFN_##fn)vk.vkGetInstanceProcAddr(instance, #fn); \
    if (!vk.fn) { LOG_WARN("vk_loader: missing instance func " #fn); }

#define LOAD_DEVICE(fn) \
    vk.fn = (PFN_##fn)vk.vkGetDeviceProcAddr(device, #fn); \
    if (!vk.fn) { LOG_WARN("vk_loader: missing device func " #fn); }

bool vk_loader_init(void) {
    memset(&vk, 0, sizeof(vk));

    s_lib = LoadLibraryA("vulkan-1.dll");
    if (!s_lib) {
        LOG_ERROR("vk_loader: vulkan-1.dll not found");
        return false;
    }

    vk.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)
        GetProcAddress(s_lib, "vkGetInstanceProcAddr");
    if (!vk.vkGetInstanceProcAddr) {
        LOG_ERROR("vk_loader: vkGetInstanceProcAddr not found");
        return false;
    }

    LOAD_GLOBAL(vkCreateInstance)
    LOAD_GLOBAL(vkEnumerateInstanceExtensionProperties)
    LOAD_GLOBAL(vkEnumerateInstanceLayerProperties)

    LOG_INFO("Vulkan loader initialized");
    return true;
}

void vk_loader_load_instance_funcs(VkInstance instance) {
    LOAD_INSTANCE(vkDestroyInstance)
    LOAD_INSTANCE(vkEnumeratePhysicalDevices)
    LOAD_INSTANCE(vkGetPhysicalDeviceProperties)
    LOAD_INSTANCE(vkGetPhysicalDeviceFeatures)
    LOAD_INSTANCE(vkGetPhysicalDeviceMemoryProperties)
    LOAD_INSTANCE(vkGetPhysicalDeviceQueueFamilyProperties)
    LOAD_INSTANCE(vkGetPhysicalDeviceSurfaceSupportKHR)
    LOAD_INSTANCE(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    LOAD_INSTANCE(vkGetPhysicalDeviceSurfaceFormatsKHR)
    LOAD_INSTANCE(vkGetPhysicalDeviceSurfacePresentModesKHR)
    LOAD_INSTANCE(vkCreateDevice)
    LOAD_INSTANCE(vkDestroySurfaceKHR)
    LOAD_INSTANCE(vkGetDeviceProcAddr)
    LOAD_INSTANCE(vkCreateDebugUtilsMessengerEXT)
    LOAD_INSTANCE(vkDestroyDebugUtilsMessengerEXT)
    LOG_INFO("Vulkan instance functions loaded");
}

void vk_loader_load_device_funcs(VkDevice device) {
    LOAD_DEVICE(vkDestroyDevice)
    LOAD_DEVICE(vkGetDeviceQueue)
    LOAD_DEVICE(vkDeviceWaitIdle)
    LOAD_DEVICE(vkCreateSwapchainKHR)
    LOAD_DEVICE(vkDestroySwapchainKHR)
    LOAD_DEVICE(vkGetSwapchainImagesKHR)
    LOAD_DEVICE(vkAcquireNextImageKHR)
    LOAD_DEVICE(vkQueuePresentKHR)
    LOAD_DEVICE(vkQueueSubmit)
    LOAD_DEVICE(vkCreateImageView)
    LOAD_DEVICE(vkDestroyImageView)
    LOAD_DEVICE(vkCreateRenderPass)
    LOAD_DEVICE(vkDestroyRenderPass)
    LOAD_DEVICE(vkCreateFramebuffer)
    LOAD_DEVICE(vkDestroyFramebuffer)
    LOAD_DEVICE(vkCreateShaderModule)
    LOAD_DEVICE(vkDestroyShaderModule)
    LOAD_DEVICE(vkCreatePipelineLayout)
    LOAD_DEVICE(vkDestroyPipelineLayout)
    LOAD_DEVICE(vkCreateGraphicsPipelines)
    LOAD_DEVICE(vkDestroyPipeline)
    LOAD_DEVICE(vkCreateCommandPool)
    LOAD_DEVICE(vkDestroyCommandPool)
    LOAD_DEVICE(vkAllocateCommandBuffers)
    LOAD_DEVICE(vkFreeCommandBuffers)
    LOAD_DEVICE(vkBeginCommandBuffer)
    LOAD_DEVICE(vkEndCommandBuffer)
    LOAD_DEVICE(vkResetCommandBuffer)
    LOAD_DEVICE(vkCmdBeginRenderPass)
    LOAD_DEVICE(vkCmdEndRenderPass)
    LOAD_DEVICE(vkCmdBindPipeline)
    LOAD_DEVICE(vkCmdBindVertexBuffers)
    LOAD_DEVICE(vkCmdBindIndexBuffer)
    LOAD_DEVICE(vkCmdDraw)
    LOAD_DEVICE(vkCmdDrawIndexed)
    LOAD_DEVICE(vkCmdSetViewport)
    LOAD_DEVICE(vkCmdSetScissor)
    LOAD_DEVICE(vkCmdCopyBuffer)
    LOAD_DEVICE(vkCmdPushConstants)
    LOAD_DEVICE(vkCreateSemaphore)
    LOAD_DEVICE(vkDestroySemaphore)
    LOAD_DEVICE(vkCreateFence)
    LOAD_DEVICE(vkDestroyFence)
    LOAD_DEVICE(vkWaitForFences)
    LOAD_DEVICE(vkResetFences)
    LOAD_DEVICE(vkCreateBuffer)
    LOAD_DEVICE(vkDestroyBuffer)
    LOAD_DEVICE(vkGetBufferMemoryRequirements)
    LOAD_DEVICE(vkAllocateMemory)
    LOAD_DEVICE(vkFreeMemory)
    LOAD_DEVICE(vkBindBufferMemory)
    LOAD_DEVICE(vkMapMemory)
    LOAD_DEVICE(vkUnmapMemory)
    LOAD_DEVICE(vkCreateImage)
    LOAD_DEVICE(vkDestroyImage)
    LOAD_DEVICE(vkGetImageMemoryRequirements)
    LOAD_DEVICE(vkBindImageMemory)
    LOAD_DEVICE(vkCreateSampler)
    LOAD_DEVICE(vkDestroySampler)
    LOAD_DEVICE(vkCreateDescriptorSetLayout)
    LOAD_DEVICE(vkDestroyDescriptorSetLayout)
    LOAD_DEVICE(vkCreateDescriptorPool)
    LOAD_DEVICE(vkDestroyDescriptorPool)
    LOAD_DEVICE(vkAllocateDescriptorSets)
    LOAD_DEVICE(vkUpdateDescriptorSets)
    LOAD_DEVICE(vkCmdPipelineBarrier)
    LOAD_DEVICE(vkCmdCopyBufferToImage)
    LOG_INFO("Vulkan device functions loaded");
}

void vk_loader_shutdown(void) {
    if (s_lib) { FreeLibrary(s_lib); s_lib = NULL; }
    memset(&vk, 0, sizeof(vk));
}