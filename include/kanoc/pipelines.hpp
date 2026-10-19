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

        std::vector<VkPushConstantRange> pushconstants;
        std::vector<VkDescriptorSetLayout> descriptorlayouts;
        VkPipelineLayoutCreateInfo layoutci;

        std::vector<VkVertexInputBindingDescription> vertexbindings;
        std::vector<VkVertexInputAttributeDescription> vertexattributes;
        VkPipelineVertexInputStateCreateInfo vertexci;

        VkPipelineInputAssemblyStateCreateInfo inputassemblyci;

        std::vector<VkPipelineShaderStageCreateInfo> shaderstagecis;

        VkPipelineViewportStateCreateInfo viewportci;

        std::vector<VkDynamicState> dynamicstates;
        VkPipelineDynamicStateCreateInfo dynamicstateci;

        VkPipelineDepthStencilStateCreateInfo depthstencilci;

        std::vector<VkFormat> attachmentformats;
        VkFormat depthformat;
        VkPipelineRenderingCreateInfo renderingci;

        std::vector<VkPipelineColorBlendAttachmentState> blendattachments;
        VkPipelineColorBlendStateCreateInfo blendci;

        VkPipelineRasterizationStateCreateInfo rasterci;

        VkPipelineMultisampleStateCreateInfo multisampleci;

        VkGraphicsPipelineCreateInfo pipelineci;

        GraphicsPipeline Build(FullDevice& vk);
        void BuildLayoutCI();
        void BuildVertexCI();
        void BuildDynamicCI();
        void BuildRenderingCI();
        void BuildBlendCI();
};
