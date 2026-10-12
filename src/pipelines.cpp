#include "kanoc/pipelines.hpp"
#include "kanoc/chk.hpp"
#include "kanoc/devices.hpp"
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
