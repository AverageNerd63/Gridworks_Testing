#pragma once
#include "../../core/types.h"
#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef struct {
    /* ---- global ---- */
    PFN_vkGetInstanceProcAddr                  vkGetInstanceProcAddr;
    PFN_vkCreateInstance                       vkCreateInstance;
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
    PFN_vkEnumerateInstanceLayerProperties     vkEnumerateInstanceLayerProperties;

    /* ---- instance ---- */
    PFN_vkDestroyInstance                         vkDestroyInstance;
    PFN_vkEnumeratePhysicalDevices                vkEnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties             vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceFeatures               vkGetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceMemoryProperties       vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties  vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR      vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateDevice                            vkCreateDevice;
    PFN_vkDestroySurfaceKHR                       vkDestroySurfaceKHR;
    PFN_vkGetDeviceProcAddr                       vkGetDeviceProcAddr;
    PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

    /* ---- device ---- */
    PFN_vkDestroyDevice               vkDestroyDevice;
    PFN_vkGetDeviceQueue              vkGetDeviceQueue;
    PFN_vkDeviceWaitIdle              vkDeviceWaitIdle;
    PFN_vkCreateSwapchainKHR          vkCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR         vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR       vkGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR         vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR             vkQueuePresentKHR;
    PFN_vkQueueSubmit                 vkQueueSubmit;
    PFN_vkCreateImageView             vkCreateImageView;
    PFN_vkDestroyImageView            vkDestroyImageView;
    PFN_vkCreateRenderPass            vkCreateRenderPass;
    PFN_vkDestroyRenderPass           vkDestroyRenderPass;
    PFN_vkCreateFramebuffer           vkCreateFramebuffer;
    PFN_vkDestroyFramebuffer          vkDestroyFramebuffer;
    PFN_vkCreateShaderModule          vkCreateShaderModule;
    PFN_vkDestroyShaderModule         vkDestroyShaderModule;
    PFN_vkCreatePipelineLayout        vkCreatePipelineLayout;
    PFN_vkDestroyPipelineLayout       vkDestroyPipelineLayout;
    PFN_vkCreateGraphicsPipelines     vkCreateGraphicsPipelines;
    PFN_vkDestroyPipeline             vkDestroyPipeline;
    PFN_vkCreateCommandPool           vkCreateCommandPool;
    PFN_vkDestroyCommandPool          vkDestroyCommandPool;
    PFN_vkAllocateCommandBuffers      vkAllocateCommandBuffers;
    PFN_vkFreeCommandBuffers          vkFreeCommandBuffers;
    PFN_vkBeginCommandBuffer          vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer            vkEndCommandBuffer;
    PFN_vkResetCommandBuffer          vkResetCommandBuffer;
    PFN_vkCmdBeginRenderPass          vkCmdBeginRenderPass;
    PFN_vkCmdEndRenderPass            vkCmdEndRenderPass;
    PFN_vkCmdBindPipeline             vkCmdBindPipeline;
    PFN_vkCmdBindVertexBuffers        vkCmdBindVertexBuffers;
    PFN_vkCmdBindIndexBuffer          vkCmdBindIndexBuffer;
    PFN_vkCmdDraw                     vkCmdDraw;
    PFN_vkCmdDrawIndexed              vkCmdDrawIndexed;
    PFN_vkCmdSetViewport              vkCmdSetViewport;
    PFN_vkCmdSetScissor               vkCmdSetScissor;
    PFN_vkCmdCopyBuffer               vkCmdCopyBuffer;
    PFN_vkCmdPushConstants            vkCmdPushConstants;
    PFN_vkCreateSemaphore             vkCreateSemaphore;
    PFN_vkDestroySemaphore            vkDestroySemaphore;
    PFN_vkCreateFence                 vkCreateFence;
    PFN_vkDestroyFence                vkDestroyFence;
    PFN_vkWaitForFences               vkWaitForFences;
    PFN_vkResetFences                 vkResetFences;
    PFN_vkCreateBuffer                vkCreateBuffer;
    PFN_vkDestroyBuffer               vkDestroyBuffer;
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
    PFN_vkAllocateMemory              vkAllocateMemory;
    PFN_vkFreeMemory                  vkFreeMemory;
    PFN_vkBindBufferMemory            vkBindBufferMemory;
    PFN_vkMapMemory                   vkMapMemory;
    PFN_vkUnmapMemory                 vkUnmapMemory;
} VkFuncTable;

extern VkFuncTable vk;

bool vk_loader_init(void);
void vk_loader_load_instance_funcs(VkInstance instance);
void vk_loader_load_device_funcs(VkDevice device);
void vk_loader_shutdown(void);