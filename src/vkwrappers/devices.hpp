#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include <vk_mem_alloc.h>

struct Extensions{
    uint32_t count;
    std::vector<const char*> items;
};

struct LogicalDevice{
    VkDevice handle = {VK_NULL_HANDLE};
    VkPhysicalDevice physical = {VK_NULL_HANDLE};
};

struct VmaFields{
    VmaAllocator allocator = {VK_NULL_HANDLE};
};

struct FullDevice{
    Extensions extensions;
    VkInstance instance = {VK_NULL_HANDLE};
    LogicalDevice ldevice;
    VmaFields vma;
};
