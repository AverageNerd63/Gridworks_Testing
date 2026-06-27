#include "vk_texture.h"
#include "vk_loader.h"
#include "vk_buffer.h"
#include "../../core/logger.h"
#include <stdlib.h>
#include <string.h>

static u32 find_memory_type(u32 type_bits, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mp;
    vk.vkGetPhysicalDeviceMemoryProperties(s_ctx.gpu, &mp);
    for (u32 i = 0; i < mp.memoryTypeCount; i++)
        if ((type_bits & (1u << i)) &&
            (mp.memoryTypes[i].propertyFlags & props) == props)
            return i;
    return UINT32_MAX;
}

static bool transition_layout(VkCommandPool pool, VkImage img,
                               VkImageLayout from, VkImageLayout to) {
    VkCommandBufferAllocateInfo alloc = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    vk.vkAllocateCommandBuffers(s_ctx.device, &alloc, &cmd);

    VkCommandBufferBeginInfo begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vk.vkBeginCommandBuffer(cmd, &begin);

    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = from,
        .newLayout           = to,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = img,
        .subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    VkPipelineStageFlags src_stage, dst_stage;
    if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    vk.vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
    vk.vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1, .pCommandBuffers = &cmd,
    };
    vk.vkQueueSubmit(s_ctx.graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vk.vkDeviceWaitIdle(s_ctx.device);
    vk.vkFreeCommandBuffers(s_ctx.device, pool, 1, &cmd);
    return true;
}

bool gw_texture_create_r8(GwTexture *tex, const u8 *pixels, u32 w, u32 h,
                           VkCommandPool cmd_pool) {
    tex->width = w; tex->height = h;
    VkDeviceSize size = (VkDeviceSize)w * h;

    /* staging buffer */
    VkBuffer       staging_buf;
    VkDeviceMemory staging_mem;
    alloc_buffer(size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buf, &staging_mem);

    void *mapped;
    vk.vkMapMemory(s_ctx.device, staging_mem, 0, size, 0, &mapped);
    memcpy(mapped, pixels, (usize)size);
    vk.vkUnmapMemory(s_ctx.device, staging_mem);

    /* image */
    VkImageCreateInfo img_ci = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = VK_FORMAT_R8_UNORM,
        .extent        = { w, h, 1 },
        .mipLevels     = 1, .arrayLayers = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    vk.vkCreateImage(s_ctx.device, &img_ci, NULL, &tex->image);

    VkMemoryRequirements mr;
    vk.vkGetImageMemoryRequirements(s_ctx.device, tex->image, &mr);
    u32 mem_type = find_memory_type(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mem_type == UINT32_MAX) {
        LOG_ERROR("[texture] no suitable device-local memory type");
        return false;
    }
    VkMemoryAllocateInfo mem_ai = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mr.size,
        .memoryTypeIndex = mem_type,
    };
    vk.vkAllocateMemory(s_ctx.device, &mem_ai, NULL, &tex->memory);
    vk.vkBindImageMemory(s_ctx.device, tex->image, tex->memory, 0);

    /* transition → copy → transition */
    transition_layout(cmd_pool, tex->image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkCommandBufferAllocateInfo alloc = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = cmd_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    vk.vkAllocateCommandBuffers(s_ctx.device, &alloc, &cmd);
    VkCommandBufferBeginInfo begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vk.vkBeginCommandBuffer(cmd, &begin);
    VkBufferImageCopy copy = {
        .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .imageExtent      = { w, h, 1 },
    };
    vk.vkCmdCopyBufferToImage(cmd, staging_buf, tex->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    vk.vkEndCommandBuffer(cmd);
    VkSubmitInfo submit = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1, .pCommandBuffers = &cmd,
    };
    vk.vkQueueSubmit(s_ctx.graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vk.vkDeviceWaitIdle(s_ctx.device);
    vk.vkFreeCommandBuffers(s_ctx.device, cmd_pool, 1, &cmd);

    transition_layout(cmd_pool, tex->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vk.vkDestroyBuffer(s_ctx.device, staging_buf, NULL);
    vk.vkFreeMemory(s_ctx.device, staging_mem, NULL);

    /* image view */
    VkImageViewCreateInfo view_ci = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = tex->image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = VK_FORMAT_R8_UNORM,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    vk.vkCreateImageView(s_ctx.device, &view_ci, NULL, &tex->view);

    /* sampler */
    VkSamplerCreateInfo samp_ci = {
        .sType      = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter  = VK_FILTER_LINEAR,
        .minFilter  = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .maxLod       = 1.0f,
    };
    vk.vkCreateSampler(s_ctx.device, &samp_ci, NULL, &tex->sampler);

    /* descriptor set layout */
    VkDescriptorSetLayoutBinding binding = {
        .binding         = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo dsl_ci = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1, .pBindings = &binding,
    };
    vk.vkCreateDescriptorSetLayout(s_ctx.device, &dsl_ci, NULL, &tex->set_layout);

    /* descriptor pool + set */
    VkDescriptorPoolSize pool_size = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1
    };
    VkDescriptorPoolCreateInfo pool_ci = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = 1, .poolSizeCount = 1, .pPoolSizes = &pool_size,
    };
    vk.vkCreateDescriptorPool(s_ctx.device, &pool_ci, NULL, &tex->pool);

    VkDescriptorSetAllocateInfo ds_ai = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = tex->pool,
        .descriptorSetCount = 1, .pSetLayouts = &tex->set_layout,
    };
    vk.vkAllocateDescriptorSets(s_ctx.device, &ds_ai, &tex->set);

    VkDescriptorImageInfo img_info = {
        .sampler     = tex->sampler,
        .imageView   = tex->view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = tex->set,
        .dstBinding      = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1, .pImageInfo = &img_info,
    };
    vk.vkUpdateDescriptorSets(s_ctx.device, 1, &write, 0, NULL);

    LOG_INFO("[texture] created R8 %ux%u", w, h);
    return true;
}

void gw_texture_destroy(GwTexture *tex) {
    if (!tex->image) return;
    vk.vkDestroyDescriptorPool(s_ctx.device, tex->pool, NULL);
    vk.vkDestroyDescriptorSetLayout(s_ctx.device, tex->set_layout, NULL);
    vk.vkDestroySampler(s_ctx.device, tex->sampler, NULL);
    vk.vkDestroyImageView(s_ctx.device, tex->view, NULL);
    vk.vkDestroyImage(s_ctx.device, tex->image, NULL);
    vk.vkFreeMemory(s_ctx.device, tex->memory, NULL);
    memset(tex, 0, sizeof(*tex));
}