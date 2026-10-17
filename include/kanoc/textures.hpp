#pragma once

#include <glm/ext/vector_int2.hpp>
#include <string>

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <ktx.h>
#include <ktxvulkan.h>

#include "kanoc/vkwrap.hpp"
#include "kanoc/devices.hpp"
#include "kanoc/buffers.hpp"
#include "kanoc/cmdbufs.hpp"

class SamplerTexture {
    public:
        VmaAllocationCreateInfo allocci{};
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VkImageCreateInfo imageci{};
        VkImage image{ VK_NULL_HANDLE };
        VkImageViewCreateInfo viewci{};
        VkImageView view{ VK_NULL_HANDLE };
        VkSamplerCreateInfo samplerci{};
        VkSampler sampler{ VK_NULL_HANDLE };

        void InitFromFile(FullDevice& vk, VkCommandPool& cbpool, GraphicsQueue& queue, std::string path);
        void InitColorReadWrite(FullDevice& vk, glm::ivec2& size);
        void Destroy(FullDevice& vk);

        void Resize(FullDevice& vk, glm::ivec2& size);

    protected:
        void createImageAndView(FullDevice& vk);
        void recordStagingCopyCommands(FullDevice& vk, OneTimeCB& cb, StagingBuffer& imgStgBuf, ktxTexture* ktxTexture);
        void createSampler(FullDevice& vk);
};

class BasicTexture {
    public:
        VmaAllocation alloc = {VK_NULL_HANDLE};
        VkImage image = {VK_NULL_HANDLE};
        VkImageView view = {VK_NULL_HANDLE};

        void Destroy(FullDevice& vk);

        static VkFormat supportedFormat(FullDevice& vk, std::vector<VkFormat>& formats, VkFormatFeatureFlags attachflag);

    protected:
        void createImageAndView(FullDevice& vk, VkImageCreateInfo& imageci, VmaAllocationCreateInfo& allocci, VkImageViewCreateInfo& viewci);
};

class DepthTexture : public BasicTexture{
    public:
        VkImageCreateInfo imageci = {};

        void Init(FullDevice& vk, glm::ivec2& size);
        void Resize(FullDevice& vk, glm::ivec2& size);

    protected:
        VmaAllocationCreateInfo getAllocci();
        VkImageViewCreateInfo getViewci();
};

class ColorTexture : public BasicTexture{
    public:
        VkImageCreateInfo imageci = {};

        void Init(FullDevice &vk, glm::ivec2& size);
};

class IndexTexture : public BasicTexture{
    public:
        void Init(FullDevice &vk, glm::ivec2& size);
        void FillBufferCMD(FullDevice  &vk);
};
