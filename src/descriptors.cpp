#include "kanoc/descriptors.hpp"
#include "kanoc/chk.hpp"
#include "kanoc/debug.hpp"
#include "kanoc/devices.hpp"
#include <cstddef>
#include <cstdint>

void DestroyVkHandle(FullDevice& vk, VkDescriptorSetLayout& descriptorlayout){
    vkDestroyDescriptorSetLayout(vk.ldevice.handle, descriptorlayout, nullptr);
}

Descriptors Descriptors::Build(VkDescriptorPool pool){
    auto descriptors = Descriptors();
    descriptors.pool = pool;
    return descriptors;
}

void Descriptors::WriteSet(FullDevice& vk, VkWriteDescriptorSet& setdata){
    vkUpdateDescriptorSets(vk.ldevice.handle, 1, &setdata, 0, nullptr);
}

void Descriptors::Destroy(FullDevice& vk){
	vkDestroyDescriptorPool(vk.ldevice.handle, this->pool, nullptr);
}

void DescriptorLayoutBuilder::Add(VkDescriptorSetLayoutBinding layout, VkDescriptorBindingFlags flag){
    layouts.push_back(layout);
    flags.push_back(flag);
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(FullDevice& vk){
    // empty vector build won't work.
    // emptiness could be useful, when we want to have a stub layout.
    // in the shader this is completely valid:
    // [[vk::binding(0,0)]]
    // [[vk::binding(0,2)]]
    // notice that set 1 is skipped.
    // when constructing the layout on the cpu, a pointer array is passed.
    // indexes are assigned from first to last.
    // but if only 2 elements are in the array, 0 and 1 indexes are given out.
    // 0 and 2 is needed, so an empty stub acts as a skip.

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagci{
       .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
       .bindingCount = static_cast<uint32_t>(flags.size()),
       .pBindingFlags = flags.data() // each flag maps to each layout binding.
    };

    VkDescriptorSetLayoutCreateInfo layoutci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &flagci, // additional flags
        .bindingCount = static_cast<uint32_t>(layouts.size()),
        .pBindings = layouts.data()
    };

    VkDescriptorSetLayout layout{VK_NULL_HANDLE};
    chk(vkCreateDescriptorSetLayout(vk.ldevice.handle, &layoutci, nullptr, &layout));

    // return of this struct is safe in Vulkan
    return layout;
}

void DescriptorPoolBuilder::Add(VkDescriptorPoolSize item){
    items.push_back(item);
}

VkDescriptorPool DescriptorPoolBuilder::Build(FullDevice& vk, uint32_t maxSets){
    VkDescriptorPoolCreateInfo poolci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets, // maximum number of SETS allocated from this pool
        .poolSizeCount = static_cast<uint32_t>(items.size()),
        .pPoolSizes = items.data()
    };

    VkDescriptorPool pool{VK_NULL_HANDLE};
    chk(vkCreateDescriptorPool(vk.ldevice.handle, &poolci, nullptr, &pool));

    return pool;
}

void DescriptorSetAllocator::Add(VkDescriptorSetLayout layout, uint32_t variablesize){
    layouts.push_back(layout);
    variablesizes.push_back(variablesize);
}

void DescriptorSetAllocator::Allocate(FullDevice& vk, Descriptors& descriptors){
    VkDescriptorSetVariableDescriptorCountAllocateInfo variableci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .descriptorSetCount = static_cast<uint32_t>(variablesizes.size()),
        .pDescriptorCounts = variablesizes.data() // each size maps to each layout. Since layouts can only have 1 variable descriptor, finding it is exact.
    };
    VkDescriptorSetAllocateInfo setci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variableci,
        .descriptorPool = descriptors.pool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()), // amount of layouts
        .pSetLayouts = layouts.data()
    };

    size_t oldsize = descriptors.sets.size();
    descriptors.sets.resize(oldsize + setci.descriptorSetCount); // add potential new elements

    VkDescriptorSet* setptr = descriptors.sets.data()+oldsize; // skip already filled elements end
    chk(vkAllocateDescriptorSets(vk.ldevice.handle, &setci, setptr));
}
