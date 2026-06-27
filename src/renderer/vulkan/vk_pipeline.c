#include "vk_pipeline.h"
#include "../../core/logger.h"
#include "../../platform/filesystem.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

VkPipelineLayout s_pipeline_layout;
VkPipeline       s_pipeline;

static bool load_spv(const char *path, u32 **out_code, usize *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) { LOG_ERROR("[vulkan] cannot open shader: %s", path); return false; }
    fseek(f, 0, SEEK_END);
    *out_size = (usize)ftell(f);
    rewind(f);
    *out_code = malloc(*out_size);
    fread(*out_code, 1, *out_size, f);
    fclose(f);
    return true;
}

static VkShaderModule create_shader_module(const u32 *code, usize size) {
    VkShaderModuleCreateInfo ci = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode    = code,
    };
    VkShaderModule mod;
    vk.vkCreateShaderModule(s_ctx.device, &ci, NULL, &mod);
    return mod;
}

bool create_pipeline(void) {
    u32 *vert_code, *frag_code;
    usize vert_size, frag_size;
    if (!load_spv("build/shaders/triangle.vert.spv", &vert_code, &vert_size)) return false;
    if (!load_spv("build/shaders/triangle.frag.spv", &frag_code, &frag_size)) return false;

    VkShaderModule vert = create_shader_module(vert_code, vert_size);
    VkShaderModule frag = create_shader_module(frag_code, frag_size);
    free(vert_code); free(frag_code);

    VkPipelineShaderStageCreateInfo stages[2] = {
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,   .module = vert, .pName = "main" },
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = frag, .pName = "main" },
    };

    VkVertexInputBindingDescription binding = {
        .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    VkVertexInputAttributeDescription attrs[2] = {
        { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT,    .offset = offsetof(Vertex, pos) },
        { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, col) },
    };
    VkPipelineVertexInputStateCreateInfo vert_input = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 1, .pVertexBindingDescriptions      = &binding,
        .vertexAttributeDescriptionCount = 2, .pVertexAttributeDescriptions    = attrs,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1, .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo raster = {
        .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode    = VK_CULL_MODE_BACK_BIT,
        .frontFace   = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth   = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState blend_attach = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo blend = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1, .pAttachments = &blend_attach,
    };

    VkDynamicState dyn_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2, .pDynamicStates = dyn_states,
    };

    VkPipelineLayoutCreateInfo layout_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    VK_CHECK(vk.vkCreatePipelineLayout(s_ctx.device, &layout_ci, NULL, &s_pipeline_layout));

    VkGraphicsPipelineCreateInfo ci = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,   .pStages             = stages,
        .pVertexInputState   = &vert_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &raster,
        .pMultisampleState   = &multisample,
        .pColorBlendState    = &blend,
        .pDynamicState       = &dynamic,
        .layout              = s_pipeline_layout,
        .renderPass          = s_render_pass,
        .subpass             = 0,
    };
    VK_CHECK(vk.vkCreateGraphicsPipelines(s_ctx.device, VK_NULL_HANDLE, 1, &ci, NULL, &s_pipeline));

    vk.vkDestroyShaderModule(s_ctx.device, vert, NULL);
    vk.vkDestroyShaderModule(s_ctx.device, frag, NULL);

    LOG_INFO("[vulkan] pipeline created");
    return true;
}

void destroy_pipeline(void) {
    if (s_pipeline)        { vk.vkDestroyPipeline(s_ctx.device,       s_pipeline,        NULL); s_pipeline        = VK_NULL_HANDLE; }
    if (s_pipeline_layout) { vk.vkDestroyPipelineLayout(s_ctx.device, s_pipeline_layout, NULL); s_pipeline_layout = VK_NULL_HANDLE; }
}