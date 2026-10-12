#pragma once

#include "kanoc/devices.hpp"
#include "vulkan/vulkan_core.h"

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
