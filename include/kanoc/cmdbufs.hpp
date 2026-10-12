#pragma once

#include <vulkan/vulkan_core.h>

#include "kanoc/vkwrap.hpp"
#include "kanoc/devices.hpp"

class OneTimeCB{
    public:
        VkFence fence;
        VkCommandBuffer handle;

        void Begin(FullDevice& vk, VkCommandPool& cbpool);
        void End(FullDevice& vk, GraphicsQueue& queue);

    protected:
        void initInBegin(FullDevice& vk, VkCommandPool& cbpool);
        void destroyInEnd(FullDevice& vk);
};
