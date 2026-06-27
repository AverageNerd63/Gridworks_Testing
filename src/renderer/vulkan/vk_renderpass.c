#include "vk_renderpass.h"
#include "../../core/logger.h"
#include <stdlib.h>

VkRenderPass   s_render_pass;
VkFramebuffer *s_framebuffers;

bool create_render_pass(void) {
    VkAttachmentDescription color = {
        .format         = s_swapchain.image_format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference color_ref = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass = {
        .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_ref,
    };
    VkSubpassDependency dep = {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    VkRenderPassCreateInfo ci = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1, .pAttachments    = &color,
        .subpassCount    = 1, .pSubpasses      = &subpass,
        .dependencyCount = 1, .pDependencies   = &dep,
    };
    VK_CHECK(vk.vkCreateRenderPass(s_ctx.device, &ci, NULL, &s_render_pass));
    LOG_INFO("[vulkan] render pass created");
    return true;
}

void destroy_render_pass(void) {
    if (s_render_pass) {
        vk.vkDestroyRenderPass(s_ctx.device, s_render_pass, NULL);
        s_render_pass = VK_NULL_HANDLE;
    }
}

bool create_framebuffers(void) {
    s_framebuffers = malloc(s_swapchain.image_count * sizeof(VkFramebuffer));
    for (u32 i = 0; i < s_swapchain.image_count; i++) {
        VkFramebufferCreateInfo ci = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = s_render_pass,
            .attachmentCount = 1,
            .pAttachments    = &s_swapchain.image_views[i],
            .width           = s_swapchain.extent.width,
            .height          = s_swapchain.extent.height,
            .layers          = 1,
        };
        VK_CHECK(vk.vkCreateFramebuffer(s_ctx.device, &ci, NULL, &s_framebuffers[i]));
    }
    LOG_INFO("[vulkan] framebuffers created (%u)", s_swapchain.image_count);
    return true;
}

void destroy_framebuffers(void) {
    if (!s_framebuffers) return;
    for (u32 i = 0; i < s_swapchain.image_count; i++)
        if (s_framebuffers[i])
            vk.vkDestroyFramebuffer(s_ctx.device, s_framebuffers[i], NULL);
    free(s_framebuffers);
    s_framebuffers = NULL;
}