#include "kanoc/pipelines.hpp"
#include "kanoc/chk.hpp"
#include "kanoc/devices.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

GraphicsPipeline GraphicsPipeline::Build(FullDevice& vk, VkPipelineLayoutCreateInfo& layoutci, VkGraphicsPipelineCreateInfo& pipelineci){
    GraphicsPipeline pipeline;

    chk(vkCreatePipelineLayout(vk.ldevice.handle, &layoutci, nullptr, &pipeline.layout));
    pipelineci.layout = pipeline.layout;
    chk(vkCreateGraphicsPipelines(vk.ldevice.handle, VK_NULL_HANDLE, 1, &pipelineci, nullptr, &pipeline.handle));

    return pipeline;
}

void GraphicsPipeline::Destroy(FullDevice& vk){
    vkDestroyPipelineLayout(vk.ldevice.handle, this->layout, nullptr);
	vkDestroyPipeline(vk.ldevice.handle, this->handle, nullptr);
}

VkPipelineColorBlendAttachmentState GraphicsPipeline::basicColorBlendAttachmentci(){
    return {
        .colorWriteMask = 0xF
    };
}

VkPipelineColorBlendStateCreateInfo GraphicsPipeline::basicColorblendci(VkPipelineColorBlendAttachmentState& blendattachmentci){
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blendattachmentci
    };
}

VkPipelineRasterizationStateCreateInfo GraphicsPipeline::basicRasterizationci(){
    return {
         .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
         .lineWidth = 1.0f
    };
}

VkPipelineMultisampleStateCreateInfo GraphicsPipeline::basicMultisampleci(){
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };
}

GraphicsPipeline PipelineBuilder::Build(FullDevice& vk){
    auto layoutci = CreateLayoutCI(defPushConstants, defDescriptorLayouts);
    auto vertexci = CreateVertexCI(defVertexBindings, defVertexAttributes);
    auto dynamicstateci = CreateDynamicCI(defDynamicStates);
    auto renderci = CreateRenderingCI(defAttachmentFormats, defDepthFormat);
    auto blendci = CreateBlendCI(defAttachmentBlends);

    // Automatic .stype setting.
    for (auto& shaderstageci : defShaderStageCIs) {
        shaderstageci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    }
    defInputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    defViewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    defRasterizationCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    defMultiSampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    defDepthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    pipelineci = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderci,
        .stageCount = static_cast<uint32_t>(defShaderStageCIs.size()),
        .pStages = defShaderStageCIs.data(),
        .pVertexInputState = &vertexci,
        .pInputAssemblyState = &defInputAssemblyCI,
        .pViewportState = &defViewportCI,
        .pRasterizationState = &defRasterizationCI,
        .pMultisampleState = &defMultiSampleCI,
        .pDepthStencilState = &defDepthStencilCI,
        .pColorBlendState = &blendci,
        .pDynamicState = &dynamicstateci,
    };

    GraphicsPipeline pipeline;

    chk(vkCreatePipelineLayout(vk.ldevice.handle, &layoutci, nullptr, &pipeline.layout));
    pipelineci.layout = pipeline.layout;
    chk(vkCreateGraphicsPipelines(vk.ldevice.handle, VK_NULL_HANDLE, 1, &pipelineci, nullptr, &pipeline.handle));

    return pipeline;
}

VkPipelineLayoutCreateInfo PipelineBuilder::CreateLayoutCI(std::vector<VkPushConstantRange>& pushconstants, std::vector<VkDescriptorSetLayout>& descriptorlayouts){
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptorlayouts.size()),
        .pSetLayouts = descriptorlayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pushconstants.size()),
        .pPushConstantRanges = pushconstants.data()
    };
}

VkPipelineVertexInputStateCreateInfo PipelineBuilder::CreateVertexCI(std::vector<VkVertexInputBindingDescription>& vertexbindings, std::vector<VkVertexInputAttributeDescription>& vertexattributes){
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexbindings.size()),
        .pVertexBindingDescriptions = vertexbindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexattributes.size()),
        .pVertexAttributeDescriptions = vertexattributes.data(),
    };
}

VkPipelineDynamicStateCreateInfo PipelineBuilder::CreateDynamicCI(std::vector<VkDynamicState>& dynamicstates){
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicstates.size()),
        .pDynamicStates = dynamicstates.data()
    };
}

VkPipelineRenderingCreateInfo PipelineBuilder::CreateRenderingCI(std::vector<VkFormat>& attachmentformats, VkFormat& depthformat){
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = static_cast<uint32_t>(attachmentformats.size()),
        .pColorAttachmentFormats = attachmentformats.data(),
        .depthAttachmentFormat = depthformat
    };
}

VkPipelineColorBlendStateCreateInfo PipelineBuilder::CreateBlendCI(std::vector<VkPipelineColorBlendAttachmentState>& blendattachments){
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(blendattachments.size()),
        .pAttachments = blendattachments.data()
    };
}
