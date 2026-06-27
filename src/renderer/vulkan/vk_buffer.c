#include "vk_buffer.h"
#include "../../core/logger.h"
#include <stdlib.h>
#include <string.h>

VkBuffer       s_vertex_buf;
VkDeviceMemory s_vertex_mem;
VkBuffer       s_index_buf;
VkDeviceMemory s_index_mem;

static u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem;
    vk.vkGetPhysicalDeviceMemoryProperties(s_ctx.gpu, &mem);
    for (u32 i = 0; i < mem.memoryTypeCount; i++)
        if ((type_filter & (1u << i)) &&
            (mem.memoryTypes[i].propertyFlags & props) == props)
            return i;
    LOG_ERROR("[vulkan] no suitable memory type");
    return UINT32_MAX;
}

bool alloc_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags props,
                  VkBuffer *out_buf, VkDeviceMemory *out_mem) {
    VkBufferCreateInfo ci = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_CHECK(vk.vkCreateBuffer(s_ctx.device, &ci, NULL, out_buf));

    VkMemoryRequirements req;
    vk.vkGetBufferMemoryRequirements(s_ctx.device, *out_buf, &req);

    u32 mem_type = find_memory_type(req.memoryTypeBits, props);
    if (mem_type == UINT32_MAX) return false;

    VkMemoryAllocateInfo alloc = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = req.size,
        .memoryTypeIndex = mem_type,
    };
    VK_CHECK(vk.vkAllocateMemory(s_ctx.device, &alloc, NULL, out_mem));
    vk.vkBindBufferMemory(s_ctx.device, *out_buf, *out_mem, 0);
    return true;
}

void upload_buffer(VkBuffer dst, const void *data, VkDeviceSize size) {
    VkBuffer       staging_buf;
    VkDeviceMemory staging_mem;
    alloc_buffer(size,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &staging_buf, &staging_mem);

    void *mapped;
    vk.vkMapMemory(s_ctx.device, staging_mem, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vk.vkUnmapMemory(s_ctx.device, staging_mem);

    VkCommandPoolCreateInfo pool_ci = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = s_ctx.graphics_family,
    };
    VkCommandPool transfer_pool;
    vk.vkCreateCommandPool(s_ctx.device, &pool_ci, NULL, &transfer_pool);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = transfer_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    vk.vkAllocateCommandBuffers(s_ctx.device, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vk.vkBeginCommandBuffer(cmd, &begin);
    VkBufferCopy region = { .size = size };
    vk.vkCmdCopyBuffer(cmd, staging_buf, dst, 1, &region);
    vk.vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd,
    };
    vk.vkQueueSubmit(s_ctx.graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vk.vkDeviceWaitIdle(s_ctx.device);

    vk.vkDestroyCommandPool(s_ctx.device, transfer_pool, NULL);
    vk.vkDestroyBuffer(s_ctx.device, staging_buf, NULL);
    vk.vkFreeMemory(s_ctx.device, staging_mem, NULL);
}

bool create_mesh_buffers(void) {
    static const Vertex verts[] = {
        {{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    };
    static const u16 indices[] = { 0, 1, 2 };

    alloc_buffer(sizeof(verts),
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &s_vertex_buf, &s_vertex_mem);
    upload_buffer(s_vertex_buf, verts, sizeof(verts));

    alloc_buffer(sizeof(indices),
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &s_index_buf, &s_index_mem);
    upload_buffer(s_index_buf, indices, sizeof(indices));

    LOG_INFO("[vulkan] mesh buffers uploaded");
    return true;
}

void destroy_mesh_buffers(void) {
    if (s_index_buf)  { vk.vkDestroyBuffer(s_ctx.device, s_index_buf,  NULL); s_index_buf  = VK_NULL_HANDLE; }
    if (s_index_mem)  { vk.vkFreeMemory(s_ctx.device,    s_index_mem,  NULL); s_index_mem  = VK_NULL_HANDLE; }
    if (s_vertex_buf) { vk.vkDestroyBuffer(s_ctx.device, s_vertex_buf, NULL); s_vertex_buf = VK_NULL_HANDLE; }
    if (s_vertex_mem) { vk.vkFreeMemory(s_ctx.device,    s_vertex_mem, NULL); s_vertex_mem = VK_NULL_HANDLE; }
}