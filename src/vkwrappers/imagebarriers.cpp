#include "vkwrappers/imagebarriers.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>

void ImageBarrierCommander::Add(VkImageMemoryBarrier2&& rbarrier){
    barriers.push_back(std::move(rbarrier));
}

void ImageBarrierCommander::RecordAndClearCommands(VkCommandBuffer cb){
    VkDependencyInfo barrierInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    };
    vkCmdPipelineBarrier2(cb, &barrierInfo);
    barriers.clear();
}
