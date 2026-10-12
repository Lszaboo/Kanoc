#pragma once

#include "vulkan/vulkan_core.h"
#include <vector>

class ImageBarrierCommander{
    public:
        std::vector<VkImageMemoryBarrier2> barriers;

        void Add(VkImageMemoryBarrier2&& rbarrier);
        void RecordAndClearCommands(VkCommandBuffer cb);
};
