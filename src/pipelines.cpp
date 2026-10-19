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
    BuildLayoutCI();
    BuildVertexCI();
    BuildDynamicCI();
    BuildRenderingCI();
    BuildBlendCI();

    pipelineci = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingci,
        .stageCount = static_cast<uint32_t>(shaderstagecis.size()),
        .pStages = shaderstagecis.data(),
        .pVertexInputState = &vertexci,
        .pInputAssemblyState = &inputassemblyci,
        .pViewportState = &viewportci,
        .pRasterizationState = &rasterci,
        .pMultisampleState = &multisampleci,
        .pDepthStencilState = &depthstencilci,
        .pColorBlendState = &blendci,
        .pDynamicState = &dynamicstateci,
    };

    GraphicsPipeline pipeline;

    chk(vkCreatePipelineLayout(vk.ldevice.handle, &layoutci, nullptr, &pipeline.layout));
    pipelineci.layout = pipeline.layout;
    chk(vkCreateGraphicsPipelines(vk.ldevice.handle, VK_NULL_HANDLE, 1, &pipelineci, nullptr, &pipeline.handle));

    return pipeline;
}

void PipelineBuilder::BuildLayoutCI(){
    layoutci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptorlayouts.size()),
        .pSetLayouts = descriptorlayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pushconstants.size()),
        .pPushConstantRanges = pushconstants.data()
    };
}

void PipelineBuilder::BuildVertexCI(){
    vertexci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexbindings.size()),
        .pVertexBindingDescriptions = vertexbindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexattributes.size()),
        .pVertexAttributeDescriptions = vertexattributes.data(),
    };
}

void PipelineBuilder::BuildDynamicCI(){
    dynamicstateci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicstates.size()),
        .pDynamicStates = dynamicstates.data()
    };
}

void PipelineBuilder::BuildRenderingCI(){
    renderingci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = static_cast<uint32_t>(attachmentformats.size()),
        .pColorAttachmentFormats = attachmentformats.data(),
        .depthAttachmentFormat = depthformat
    };
}

void PipelineBuilder::BuildBlendCI(){
    blendci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(blendattachments.size()),
        .pAttachments = blendattachments.data()
    };
}
