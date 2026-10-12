#include "dynamicrendering.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>

void DynamicRenderingCommander::Add(VkRenderingAttachmentInfo&& rattachment){
    attachments.push_back(std::move(rattachment));
}

void DynamicRenderingCommander::BeginDynamicRendering(VkCommandBuffer& cb, glm::ivec2& renderArea){
    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea{.extent{.width = static_cast<uint32_t>(renderArea.x), .height = static_cast<uint32_t>(renderArea.y) }},
        .layerCount = 1,
        .colorAttachmentCount = static_cast<uint32_t>(attachments.size()),
        .pColorAttachments = attachments.data(),
        .pDepthAttachment = &depth
    };
    vkCmdBeginRendering(cb, &renderingInfo);
    attachments.clear();
}

void DynamicRenderingCommander::EndDynamicRendering(VkCommandBuffer& cb){
    vkCmdEndRendering(cb);
}
