#include "vk_frame.h"
#include "../../ui/imgui_bridge.h"
#include "../../core/logger.h"
#include <string.h>

Frame      s_frames[FRAMES_IN_FLIGHT];
u32        s_frame_idx  = 0;
u32        s_image_idx  = 0;
VkSemaphore s_acquire_sems[MAX_SWAPCHAIN_IMAGES];
VkSemaphore s_render_finished[MAX_SWAPCHAIN_IMAGES];

bool create_frames(void) {
    VkSemaphoreCreateInfo sem_ci   = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo     fence_ci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkCommandPoolCreateInfo pool_ci = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = s_ctx.graphics_family,
        };
        VK_CHECK(vk.vkCreateCommandPool(s_ctx.device, &pool_ci, NULL, &s_frames[i].cmd_pool));

        VkCommandBufferAllocateInfo alloc = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = s_frames[i].cmd_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VK_CHECK(vk.vkAllocateCommandBuffers(s_ctx.device, &alloc, &s_frames[i].cmd_buf));
        VK_CHECK(vk.vkCreateFence(s_ctx.device, &fence_ci, NULL, &s_frames[i].in_flight));
    }

    for (u32 i = 0; i < s_swapchain.image_count; i++) {
        VK_CHECK(vk.vkCreateSemaphore(s_ctx.device, &sem_ci, NULL, &s_acquire_sems[i]));
        VK_CHECK(vk.vkCreateSemaphore(s_ctx.device, &sem_ci, NULL, &s_render_finished[i]));
    }

    LOG_INFO("[vulkan] frame sync created (%d frames, %u images)", FRAMES_IN_FLIGHT, s_swapchain.image_count);
    return true;
}

void destroy_frames(void) {
    for (u32 i = 0; i < s_swapchain.image_count; i++) {
        if (s_acquire_sems[i])    { vk.vkDestroySemaphore(s_ctx.device, s_acquire_sems[i],    NULL); s_acquire_sems[i]    = VK_NULL_HANDLE; }
        if (s_render_finished[i]) { vk.vkDestroySemaphore(s_ctx.device, s_render_finished[i], NULL); s_render_finished[i] = VK_NULL_HANDLE; }
    }
    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        if (s_frames[i].in_flight) vk.vkDestroyFence(s_ctx.device,       s_frames[i].in_flight, NULL);
        if (s_frames[i].cmd_pool)  vk.vkDestroyCommandPool(s_ctx.device, s_frames[i].cmd_pool,  NULL);
    }
    memset(s_frames, 0, sizeof(s_frames));
    s_frame_idx = 0;
    s_image_idx = 0;
}

bool vk_begin_frame(void) {
    Frame *f = &s_frames[s_frame_idx];
    vk.vkWaitForFences(s_ctx.device, 1, &f->in_flight, VK_TRUE, UINT64_MAX);

    /* one acquire semaphore per frame-in-flight, not per image */
    VkSemaphore acquire_sem = s_acquire_sems[s_frame_idx];

    VkResult r = vk.vkAcquireNextImageKHR(s_ctx.device, s_swapchain.swapchain,
                                           UINT64_MAX, acquire_sem,
                                           VK_NULL_HANDLE, &s_image_idx);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) return false;
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        LOG_ERROR("[vulkan] vkAcquireNextImageKHR: %d", r);
        return false;
    }

    vk.vkResetFences(s_ctx.device, 1, &f->in_flight);
    vk.vkResetCommandBuffer(f->cmd_buf, 0);

    VkCommandBufferBeginInfo begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vk.vkBeginCommandBuffer(f->cmd_buf, &begin);

    VkClearValue clear = { .color = { .float32 = { 0.1f, 0.1f, 0.1f, 1.0f } } };
    VkRenderPassBeginInfo rp = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass      = s_render_pass,
        .framebuffer     = s_framebuffers[s_image_idx],
        .renderArea      = { .offset = {0,0}, .extent = s_swapchain.extent },
        .clearValueCount = 1,
        .pClearValues    = &clear,
    };
    vk.vkCmdBeginRenderPass(f->cmd_buf, &rp, VK_SUBPASS_CONTENTS_INLINE);

    vk.vkCmdBindPipeline(f->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipeline);

    VkViewport vp = {
        .x = 0.0f, .y = 0.0f,
        .width    = (f32)s_swapchain.extent.width,
        .height   = (f32)s_swapchain.extent.height,
        .minDepth = 0.0f, .maxDepth = 1.0f,
    };
    VkRect2D sc = { .offset = {0,0}, .extent = s_swapchain.extent };
    vk.vkCmdSetViewport(f->cmd_buf, 0, 1, &vp);
    vk.vkCmdSetScissor(f->cmd_buf, 0, 1, &sc);

    VkDeviceSize offset = 0;
    vk.vkCmdBindVertexBuffers(f->cmd_buf, 0, 1, &s_vertex_buf, &offset);
    vk.vkCmdBindIndexBuffer(f->cmd_buf, s_index_buf, 0, VK_INDEX_TYPE_UINT16);

    f->_acquire_sem = acquire_sem;

    imgui_new_frame();
    return true;
}

void vk_end_frame(void) {
    Frame *f = &s_frames[s_frame_idx];

    vk.vkCmdDrawIndexed(f->cmd_buf, 3, 1, 0, 0, 0);
    imgui_render(f->cmd_buf);

    vk.vkCmdEndRenderPass(f->cmd_buf);
    vk.vkEndCommandBuffer(f->cmd_buf);

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1, .pWaitSemaphores   = &f->_acquire_sem,
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1, .pCommandBuffers   = &f->cmd_buf,
        .signalSemaphoreCount = 1, .pSignalSemaphores = &s_render_finished[s_image_idx],
    };
    vk.vkQueueSubmit(s_ctx.graphics_queue, 1, &submit, f->in_flight);

    VkPresentInfoKHR present = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1, .pWaitSemaphores = &s_render_finished[s_image_idx],
        .swapchainCount     = 1, .pSwapchains     = &s_swapchain.swapchain,
        .pImageIndices      = &s_image_idx,
    };
    vk.vkQueuePresentKHR(s_ctx.present_queue, &present);

    s_frame_idx = (s_frame_idx + 1) % FRAMES_IN_FLIGHT;
}