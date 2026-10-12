#pragma once

#include "kanoc/devices.hpp"
#include "vulkan/vulkan_core.h"

#include <cstdint>
#include <vector>

void DestroyVkHandle(FullDevice& vk, VkDescriptorSetLayout& descriptorlayout);

class Descriptors{
    public:
        VkDescriptorPool pool = { VK_NULL_HANDLE }; // pool containing sets
        std::vector<VkDescriptorSet> sets = {}; // sets from the pool

        static Descriptors Build(VkDescriptorPool pool);
        void WriteSet(FullDevice& vk, VkWriteDescriptorSet& setdata);
        void Destroy(FullDevice& vk);
};

class DescriptorLayoutBuilder{
    public:
        std::vector<VkDescriptorSetLayoutBinding> layouts;
        std::vector<VkDescriptorBindingFlags> flags;

        void Add(VkDescriptorSetLayoutBinding layout, VkDescriptorBindingFlags flag);
        VkDescriptorSetLayout Build(FullDevice& vk);
};

class DescriptorPoolBuilder{
    public:
        std::vector<VkDescriptorPoolSize> items;

        void Add(VkDescriptorPoolSize item);
        VkDescriptorPool Build(FullDevice& vk, uint32_t maxSets);
};

class DescriptorSetAllocator{
    public:
        std::vector<VkDescriptorSetLayout> layouts;
        std::vector<uint32_t> variablesizes;

        void Add(VkDescriptorSetLayout layout, uint32_t variablesize);
        void Allocate(FullDevice& vk, Descriptors& descriptors);
};
