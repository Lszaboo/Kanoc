#include "kanoc/cmdbufs.hpp"

#include "kanoc/chk.hpp"
#include "kanoc/devices.hpp"

void OneTimeCB::initInBegin(FullDevice& vk, VkCommandPool& cbpool){
    VkFenceCreateInfo fenceOneTimeCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    chk(vkCreateFence(vk.ldevice.handle, &fenceOneTimeCI, nullptr, &this->fence));

    VkCommandBufferAllocateInfo cbOneTimeAI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cbpool, // pool must allow resets, and have space for at least 1 cbs.
        .commandBufferCount = 1
    };
    chk(vkAllocateCommandBuffers(vk.ldevice.handle, &cbOneTimeAI, &this->handle));
}

void OneTimeCB::destroyInEnd(FullDevice& vk){
    vkDestroyFence(vk.ldevice.handle, this->fence, nullptr);
}

void OneTimeCB::Begin(FullDevice& vk, VkCommandPool &cbpool){
    this->initInBegin(vk, cbpool);

    // begin recording
    VkCommandBufferBeginInfo cbOneTimeBI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    chk(vkBeginCommandBuffer(this->handle, &cbOneTimeBI));
}

void OneTimeCB::End(FullDevice& vk, GraphicsQueue& queue){
    // end recording
    chk(vkEndCommandBuffer(this->handle));

    // submit recorded commands
    VkCommandBufferSubmitInfo cbOneTimeSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = this->handle
    };
    VkSubmitInfo2 oneTimeSI{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cbOneTimeSubmitInfo
    };
    chk(vkQueueSubmit2(queue.handle, 1, &oneTimeSI, this->fence));

    // wait for commands
    chk(vkWaitForFences(vk.ldevice.handle, 1, &this->fence, VK_TRUE, UINT64_MAX));

    this->destroyInEnd(vk);
}
