#pragma once
#include "vk_ctx.h"
#include <stdbool.h>

typedef struct {
    f32 pos[2];
    f32 col[3];
} Vertex;

extern VkBuffer       s_vertex_buf;
extern VkBuffer       s_index_buf;

bool alloc_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags props,
                  VkBuffer *out_buf, VkDeviceMemory *out_mem);
void upload_buffer(VkBuffer dst, const void *data, VkDeviceSize size);
bool create_mesh_buffers(void);
void destroy_mesh_buffers(void);