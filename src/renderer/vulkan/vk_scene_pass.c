#include "vk_scene_pass.h"
#include "vk_loader.h"
#include "../../ui/imgui_bridge.h"
#include "../../core/logger.h"
#include <string.h>

ScenePass s_scene_pass;

static u32  s_pending_w       = 0;
static u32  s_pending_h       = 0;
static bool s_resize_pending  = false;

void scene_pass_request_resize(u32 w, u32 h) {
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    if (w == s_scene_pass.width && h == s_scene_pass.height) return;
    s_pending_w      = w;
    s_pending_h      = h;
    s_resize_pending = true;
}

bool scene_pass_flush_resize(void) {
    if (!s_resize_pending) return true;
    s_resize_pending = false;
    return scene_pass_resize(s_pending_w, s_pending_h);
}

#define SCENE_FORMAT VK_FORMAT_R8G8B8A8_UNORM

static u32 find_mem_type(u32 bits, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mp;
    vk.vkGetPhysicalDeviceMemoryProperties(s_ctx.gpu, &mp);
    for (u32 i = 0; i < mp.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & props) == props)
            return i;
    return UINT32_MAX;
}

static bool create_image_resources(u32 w, u32 h) {
    /* color image */
    VkImageCreateInfo img_ci = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = SCENE_FORMAT,
        .extent        = { w, h, 1 },
        .mipLevels     = 1, .arrayLayers = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (vk.vkCreateImage(s_ctx.device, &img_ci, NULL, &s_scene_pass.color_image) != VK_SUCCESS)
        return false;

    VkMemoryRequirements mr;
    vk.vkGetImageMemoryRequirements(s_ctx.device, s_scene_pass.color_image, &mr);
    u32 mem_type = find_mem_type(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mem_type == UINT32_MAX) { LOG_ERROR("[scene_pass] no device-local memory"); return false; }

    VkMemoryAllocateInfo mem_ai = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mr.size,
        .memoryTypeIndex = mem_type,
    };
    vk.vkAllocateMemory(s_ctx.device, &mem_ai, NULL, &s_scene_pass.color_memory);
    vk.vkBindImageMemory(s_ctx.device, s_scene_pass.color_image, s_scene_pass.color_memory, 0);

    VkImageViewCreateInfo view_ci = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = s_scene_pass.color_image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = SCENE_FORMAT,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    vk.vkCreateImageView(s_ctx.device, &view_ci, NULL, &s_scene_pass.color_view);

    VkFramebufferCreateInfo fb_ci = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = s_scene_pass.render_pass,
        .attachmentCount = 1, .pAttachments = &s_scene_pass.color_view,
        .width = w, .height = h, .layers = 1,
    };
    vk.vkCreateFramebuffer(s_ctx.device, &fb_ci, NULL, &s_scene_pass.framebuffer);

    s_scene_pass.imgui_ds = imgui_register_texture(
        s_scene_pass.sampler, s_scene_pass.color_view);

    return true;
}

static void destroy_image_resources(void) {
    if (s_scene_pass.imgui_ds) {
        imgui_unregister_texture(s_scene_pass.imgui_ds);
        s_scene_pass.imgui_ds = NULL;
    }
    if (s_scene_pass.framebuffer)  { vk.vkDestroyFramebuffer(s_ctx.device, s_scene_pass.framebuffer,  NULL); s_scene_pass.framebuffer  = VK_NULL_HANDLE; }
    if (s_scene_pass.color_view)   { vk.vkDestroyImageView(s_ctx.device,   s_scene_pass.color_view,   NULL); s_scene_pass.color_view   = VK_NULL_HANDLE; }
    if (s_scene_pass.color_image)  { vk.vkDestroyImage(s_ctx.device,       s_scene_pass.color_image,  NULL); s_scene_pass.color_image  = VK_NULL_HANDLE; }
    if (s_scene_pass.color_memory) { vk.vkFreeMemory(s_ctx.device,         s_scene_pass.color_memory, NULL); s_scene_pass.color_memory = VK_NULL_HANDLE; }
}

bool scene_pass_create(u32 w, u32 h) {
    memset(&s_scene_pass, 0, sizeof(s_scene_pass));
    s_scene_pass.width = w; s_scene_pass.height = h;

    /* render pass — final layout is SHADER_READ_ONLY so the transition
       happens automatically at render pass end, no explicit barrier needed */
    VkAttachmentDescription color_att = {
        .format         = SCENE_FORMAT,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkAttachmentReference color_ref = {
        .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass = {
        .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1, .pColorAttachments = &color_ref,
    };
    VkSubpassDependency deps[2] = {
        {
            .srcSubpass    = VK_SUBPASS_EXTERNAL, .dstSubpass    = 0,
            .srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        {
            .srcSubpass    = 0, .dstSubpass    = VK_SUBPASS_EXTERNAL,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
    };
    VkRenderPassCreateInfo rp_ci = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1, .pAttachments = &color_att,
        .subpassCount    = 1, .pSubpasses   = &subpass,
        .dependencyCount = 2, .pDependencies = deps,
    };
    if (vk.vkCreateRenderPass(s_ctx.device, &rp_ci, NULL, &s_scene_pass.render_pass) != VK_SUCCESS) {
        LOG_ERROR("[scene_pass] failed to create render pass"); return false;
    }

    /* sampler (shared across resize cycles) */
    VkSamplerCreateInfo samp_ci = {
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_LINEAR, .minFilter = VK_FILTER_LINEAR,
        .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .maxLod       = 1.0f,
    };
    vk.vkCreateSampler(s_ctx.device, &samp_ci, NULL, &s_scene_pass.sampler);

    if (!create_image_resources(w, h)) return false;

    LOG_INFO("[scene_pass] created %ux%u", w, h);
    return true;
}

void scene_pass_destroy(void) {
    destroy_image_resources();
    if (s_scene_pass.sampler)     { vk.vkDestroySampler(s_ctx.device,     s_scene_pass.sampler,     NULL); s_scene_pass.sampler     = VK_NULL_HANDLE; }
    if (s_scene_pass.render_pass) { vk.vkDestroyRenderPass(s_ctx.device,  s_scene_pass.render_pass, NULL); s_scene_pass.render_pass = VK_NULL_HANDLE; }
    memset(&s_scene_pass, 0, sizeof(s_scene_pass));
}

bool scene_pass_resize(u32 w, u32 h) {
    vk.vkDeviceWaitIdle(s_ctx.device);
    destroy_image_resources();
    s_scene_pass.width = w; s_scene_pass.height = h;
    if (!create_image_resources(w, h)) return false;
    LOG_INFO("[scene_pass] resized %ux%u", w, h);
    return true;
}

void scene_pass_begin(VkCommandBuffer cmd) {
    VkClearValue clear = { .color = { .float32 = { 0.18f, 0.18f, 0.22f, 1.0f } } };
    VkRenderPassBeginInfo rp = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass      = s_scene_pass.render_pass,
        .framebuffer     = s_scene_pass.framebuffer,
        .renderArea      = { .extent = { s_scene_pass.width, s_scene_pass.height } },
        .clearValueCount = 1, .pClearValues = &clear,
    };
    vk.vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
}

void scene_pass_end(VkCommandBuffer cmd) {
    vk.vkCmdEndRenderPass(cmd);
    /* layout transition to SHADER_READ_ONLY happens automatically
       via finalLayout in the render pass attachment description */
}

void *scene_pass_get_imgui_id(void) {
    return s_scene_pass.imgui_ds;
}