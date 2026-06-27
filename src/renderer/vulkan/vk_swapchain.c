#include "vk_swapchain.h"
#include "../../core/logger.h"
#include <stdlib.h>
#include <string.h>

Swapchain s_swapchain;

static VkSurfaceFormatKHR choose_surface_format(VkSurfaceFormatKHR *formats, u32 count) {
    for (u32 i = 0; i < count; i++)
        if (formats[i].format     == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return formats[i];
    return formats[0];
}

static VkPresentModeKHR choose_present_mode(VkPresentModeKHR *modes, u32 count) {
    for (u32 i = 0; i < count; i++)
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) return modes[i];
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR *caps, u32 w, u32 h) {
    if (caps->currentExtent.width != UINT32_MAX) return caps->currentExtent;
    VkExtent2D e = { w, h };
    if (e.width  < caps->minImageExtent.width)  e.width  = caps->minImageExtent.width;
    if (e.width  > caps->maxImageExtent.width)  e.width  = caps->maxImageExtent.width;
    if (e.height < caps->minImageExtent.height) e.height = caps->minImageExtent.height;
    if (e.height > caps->maxImageExtent.height) e.height = caps->maxImageExtent.height;
    return e;
}

void destroy_swapchain(void) {
    if (!s_swapchain.swapchain) return;
    for (u32 i = 0; i < s_swapchain.image_count; i++)
        if (s_swapchain.image_views[i])
            vk.vkDestroyImageView(s_ctx.device, s_swapchain.image_views[i], NULL);
    free(s_swapchain.image_views);
    free(s_swapchain.images);
    vk.vkDestroySwapchainKHR(s_ctx.device, s_swapchain.swapchain, NULL);
    memset(&s_swapchain, 0, sizeof(s_swapchain));
}

bool create_swapchain(u32 w, u32 h) {
    VkSurfaceCapabilitiesKHR caps;
    vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_ctx.gpu, s_ctx.surface, &caps);

    u32 fmt_count = 0;
    vk.vkGetPhysicalDeviceSurfaceFormatsKHR(s_ctx.gpu, s_ctx.surface, &fmt_count, NULL);
    VkSurfaceFormatKHR *formats = malloc(fmt_count * sizeof(VkSurfaceFormatKHR));
    vk.vkGetPhysicalDeviceSurfaceFormatsKHR(s_ctx.gpu, s_ctx.surface, &fmt_count, formats);

    u32 mode_count = 0;
    vk.vkGetPhysicalDeviceSurfacePresentModesKHR(s_ctx.gpu, s_ctx.surface, &mode_count, NULL);
    VkPresentModeKHR *modes = malloc(mode_count * sizeof(VkPresentModeKHR));
    vk.vkGetPhysicalDeviceSurfacePresentModesKHR(s_ctx.gpu, s_ctx.surface, &mode_count, modes);

    VkSurfaceFormatKHR fmt  = choose_surface_format(formats, fmt_count);
    VkPresentModeKHR   mode = choose_present_mode(modes, mode_count);
    VkExtent2D         ext  = choose_extent(&caps, w, h);

    free(formats);
    free(modes);

    if (ext.width == 0 || ext.height == 0) return true;

    u32 image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
        image_count = caps.maxImageCount;

    u32  families[2] = { s_ctx.graphics_family, s_ctx.present_family };
    bool shared      = s_ctx.graphics_family == s_ctx.present_family;

    VkSwapchainCreateInfoKHR ci = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = s_ctx.surface,
        .minImageCount         = image_count,
        .imageFormat           = fmt.format,
        .imageColorSpace       = fmt.colorSpace,
        .imageExtent           = ext,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = shared ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = shared ? 0 : 2,
        .pQueueFamilyIndices   = shared ? NULL : families,
        .preTransform          = caps.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = mode,
        .clipped               = VK_TRUE,
    };

    VK_CHECK(vk.vkCreateSwapchainKHR(s_ctx.device, &ci, NULL, &s_swapchain.swapchain));
    s_swapchain.image_format = fmt.format;
    s_swapchain.extent       = ext;

    vk.vkGetSwapchainImagesKHR(s_ctx.device, s_swapchain.swapchain, &s_swapchain.image_count, NULL);
    s_swapchain.images      = malloc(s_swapchain.image_count * sizeof(VkImage));
    s_swapchain.image_views = malloc(s_swapchain.image_count * sizeof(VkImageView));
    vk.vkGetSwapchainImagesKHR(s_ctx.device, s_swapchain.swapchain, &s_swapchain.image_count, s_swapchain.images);

    for (u32 i = 0; i < s_swapchain.image_count; i++) {
        VkImageViewCreateInfo iv = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = s_swapchain.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = s_swapchain.image_format,
            .components = {
                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0, .levelCount     = 1,
                .baseArrayLayer = 0, .layerCount     = 1,
            },
        };
        VK_CHECK(vk.vkCreateImageView(s_ctx.device, &iv, NULL, &s_swapchain.image_views[i]));
    }

    LOG_INFO("[vulkan] swapchain %ux%u  images: %u  format: %d  mode: %d",
             ext.width, ext.height, s_swapchain.image_count, fmt.format, mode);
    return true;
}