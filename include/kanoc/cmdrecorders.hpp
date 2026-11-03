#pragma once

#include <vulkan/vulkan_core.h>

#include "dynamicrendering.hpp"
#include "imagebarriers.hpp"


class CommandRecorders{
    public:
        VkCommandBuffer cb = {VK_NULL_HANDLE};
        ImageBarrierCommander ibc{};
        DynamicRenderingCommander drc{};
};
