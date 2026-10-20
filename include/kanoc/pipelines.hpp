#pragma once

#include "kanoc/devices.hpp"
#include "vulkan/vulkan_core.h"
#include <vector>

class GraphicsPipeline{
    public:
        VkPipelineLayout layout = { VK_NULL_HANDLE }; // descriptor info
        VkPipeline handle = { VK_NULL_HANDLE }; // graphics pipeline

        static GraphicsPipeline Build(FullDevice& vk, VkPipelineLayoutCreateInfo& layoutci, VkGraphicsPipelineCreateInfo& pipelineci);
        void Destroy(FullDevice& vk);

        static VkPipelineColorBlendAttachmentState basicColorBlendAttachmentci();
        static VkPipelineColorBlendStateCreateInfo basicColorblendci(VkPipelineColorBlendAttachmentState& blendattachmentci);
        static VkPipelineRasterizationStateCreateInfo basicRasterizationci();
        static VkPipelineMultisampleStateCreateInfo basicMultisampleci();
};

class PipelineBuilder{
    public:
        std::vector<VkPushConstantRange> defPushConstants;
        std::vector<VkDescriptorSetLayout> defDescriptorLayouts;

        std::vector<VkVertexInputBindingDescription> defVertexBindings;
        std::vector<VkVertexInputAttributeDescription> defVertexAttributes;
        VkPipelineInputAssemblyStateCreateInfo defInputAssemblyCI;

        std::vector<VkPipelineShaderStageCreateInfo> defShaderStageCIs;

        VkPipelineViewportStateCreateInfo defViewportCI;
        std::vector<VkDynamicState> defDynamicStates;

        std::vector<VkFormat> defAttachmentFormats;
        std::vector<VkPipelineColorBlendAttachmentState> defAttachmentBlends;

        VkPipelineDepthStencilStateCreateInfo defDepthStencilCI;
        VkFormat defDepthFormat;

        VkPipelineRasterizationStateCreateInfo defRasterizationCI;
        VkPipelineMultisampleStateCreateInfo defMultiSampleCI;

        GraphicsPipeline Build(FullDevice& vk);

        static VkPipelineLayoutCreateInfo CreateLayoutCI(std::vector<VkPushConstantRange>& pushconstants, std::vector<VkDescriptorSetLayout>& descriptorlayouts);
        static VkPipelineVertexInputStateCreateInfo CreateVertexCI(std::vector<VkVertexInputBindingDescription>& vertexbindings, std::vector<VkVertexInputAttributeDescription>& vertexattributes);
        static VkPipelineDynamicStateCreateInfo CreateDynamicCI(std::vector<VkDynamicState>& dynamicstates);
        static VkPipelineRenderingCreateInfo CreateRenderingCI(std::vector<VkFormat>& attachmentformats, VkFormat& depthformat);
        static VkPipelineColorBlendStateCreateInfo CreateBlendCI(std::vector<VkPipelineColorBlendAttachmentState>& blendattachments);

    protected:
        VkGraphicsPipelineCreateInfo pipelineci;
};
