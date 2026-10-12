#pragma once

#include "vulkan/vulkan_core.h"
#include <glm/glm.hpp>
#include <vector>

class DynamicRenderingCommander{
    public:
        std::vector<VkRenderingAttachmentInfo> attachments;
        VkRenderingAttachmentInfo depth{};

        void Add(VkRenderingAttachmentInfo&& rattachment);
        void BeginDynamicRendering(VkCommandBuffer& cb, glm::ivec2& renderArea);
        void EndDynamicRendering(VkCommandBuffer& cb);
};
