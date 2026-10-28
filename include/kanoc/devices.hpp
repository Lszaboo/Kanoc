#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include <vk_mem_alloc.h>

struct Extensions{
    uint32_t count;
    std::vector<const char*> items;
};

class LogicalDevice{
    public:
        VkDevice handle = {VK_NULL_HANDLE};
        VkPhysicalDevice physical = {VK_NULL_HANDLE};

        void Destroy();
};

class VmaFields{
    public:
        VmaAllocator allocator = {VK_NULL_HANDLE};

        void Destroy();
};

void DestroyVKHandle(VkInstance instance);

class FullDevice{
    public:
        Extensions extensions;
        VkInstance instance = {VK_NULL_HANDLE};
        LogicalDevice ldevice;
        VmaFields vma;

        void Destoy();
};
