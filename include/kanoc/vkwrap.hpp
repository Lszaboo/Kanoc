#pragma once

#include "kanoc/devices.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <vector>

struct GraphicsQueue{
    VkQueue handle = {VK_NULL_HANDLE};
    uint32_t idx = 0; // QueueFamilyIndex
};
