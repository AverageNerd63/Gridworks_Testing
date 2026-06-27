#pragma once
#include "vk_ctx.h"
#include "vk_renderpass.h"
#include "vk_buffer.h"
#include <stdbool.h>

extern VkPipelineLayout s_pipeline_layout;
extern VkPipeline       s_pipeline;

bool create_pipeline(void);
void destroy_pipeline(void);