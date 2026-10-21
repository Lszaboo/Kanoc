#include "kanoc/textures.hpp"

#include <cstring>
#include <glm/ext/vector_int2.hpp>
#include <slang.h>
#include <string>
#include <vector>
#include <ktx.h>
#include <ktxvulkan.h>
#include <vulkan/vulkan_core.h>

#include "kanoc/chk.hpp"
#include "kanoc/cmdhelpers.hpp"
#include "kanoc/devices.hpp"
#include "kanoc/buffers.hpp"
#include "kanoc/cmdbufs.hpp"
#include "kanoc/imagebarriers.hpp"

// SampledTexture

void SamplerTexture::InitFromFile(FullDevice& vk, VkCommandPool& cbpool, GraphicsQueue& queue ,std::string path){
    ktxTexture* ktxTexture{ nullptr };
    ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

    imageci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = ktxTexture_GetVkFormat(ktxTexture),
        .extent = {.width = ktxTexture->baseWidth, .height = ktxTexture->baseHeight, .depth = 1 },
        .mipLevels = ktxTexture->numLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    viewci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageci.format,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = imageci.mipLevels, .layerCount = 1 }
    };
    allocci = {
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    createImageAndView(vk);

    // staging buffer for image
    StagingBuffer imgStgBuf = StagingBuffer::BuildForUpload(vk, (uint32_t)ktxTexture->dataSize, (char*)ktxTexture->pData);

    // cb
    OneTimeCB cb;
    cb.Begin(vk, cbpool);

    recordStagingCopyCommands(vk, cb, imgStgBuf, ktxTexture);

    cb.End(vk, queue);

    // sampler
    samplerci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 8.0f, // 8 is a widely supported value for max anisotropy
        .maxLod = (float)ktxTexture->numLevels,
    };
    createSampler(vk);

    // destroy staging buffer
    imgStgBuf.Destroy(vk);
    ktxTexture_Destroy(ktxTexture);
}

void SamplerTexture::InitColorReadWrite(FullDevice& vk, glm::ivec2& size){
    imageci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {
            .width  = static_cast<uint32_t>(size.x),
            .height = static_cast<uint32_t>(size.y),
            .depth  = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // if read from as well.
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // image is owned by one queue family.
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    allocci = {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    viewci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageci.format,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = imageci.mipLevels, .layerCount = 1 }
    };
    createImageAndView(vk);

    samplerci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnable = VK_FALSE,
        .maxLod = 0.0f,
    };
    createSampler(vk);
}

void SamplerTexture::InitIDReadWrite(FullDevice &vk, glm::ivec2 &size){
    std::vector<VkFormat> formats = {VK_FORMAT_R32_UINT};
    VkFormat format = BasicTexture::supportedFormat(vk, formats, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
    chk(format);

    imageci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {
            .width = static_cast<uint32_t>(size.x),
            .height = static_cast<uint32_t>(size.y),
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // Sample and copy the image to buffer as well.
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    allocci = {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    viewci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageci.format,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = imageci.mipLevels, .layerCount = 1 }
    };
    createImageAndView(vk);

    samplerci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnable = VK_FALSE,
        .maxLod = 0.0f,
    };
    createSampler(vk);
}

void SamplerTexture::Destroy(FullDevice& vk){
    vkDestroyImageView(vk.ldevice.handle, this->view, nullptr);
	vkDestroySampler(vk.ldevice.handle, this->sampler, nullptr);
	vmaDestroyImage(vk.vma.allocator, this->image, this->allocation);
}

void SamplerTexture::createImageAndView(FullDevice& vk){
    chk(vmaCreateImage(vk.vma.allocator, &imageci, &allocci, &image, &allocation, nullptr));
    viewci.image = image;
    chk(vkCreateImageView(vk.ldevice.handle, &viewci, nullptr, &view));
}

void SamplerTexture::recordStagingCopyCommands(FullDevice& vk, OneTimeCB& cb, StagingBuffer& imgStgBuf, ktxTexture* ktxTexture){
    // fst command
    // change texture layout from previously undefined.
    ImageBarrierCommander ibc;
    ibc.Add(
        {
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // we will transfer data to this image.
            .image = this->image,
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
        }
    );
    ibc.RecordAndClearCommands(cb.handle);

    // snd commands
    // copy mip levels from buffers to image
    std::vector<VkBufferImageCopy> copyRegions{};
    for (auto j = 0; j < ktxTexture->numLevels; j++) {
        ktx_size_t mipOffset{0};
        KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
        copyRegions.push_back({
            .bufferOffset = mipOffset,
            .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1},
            .imageExtent{.width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 },
        });
    }
    vkCmdCopyBufferToImage(
        cb.handle, imgStgBuf.buf.handle, this->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data()
    );

    // thrd command
    // change image layout again to be read-only from shader.
    ibc.Add(
        {
            .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, // read only from shader.
            .image = this->image,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
        }
    );
    ibc.RecordAndClearCommands(cb.handle);
}

void SamplerTexture::createSampler(FullDevice& vk){
    chk(vkCreateSampler(vk.ldevice.handle, &samplerci, nullptr, &sampler));
}

void SamplerTexture::Resize(FullDevice &vk, glm::ivec2& size){
    vkDestroyImageView(vk.ldevice.handle, view, nullptr);
    vmaDestroyImage(vk.vma.allocator, image, allocation);

    imageci.extent = { .width = static_cast<uint32_t>(size.x), .height = static_cast<uint32_t>(size.y), .depth = 1 };

    createImageAndView(vk);
}

// BasicTexture

void BasicTexture::createImageAndView(FullDevice& vk, VkImageCreateInfo& imageci, VmaAllocationCreateInfo& allocci, VkImageViewCreateInfo& viewci){
    chk(vmaCreateImage(vk.vma.allocator, &imageci, &allocci, &this->image, &this->alloc, nullptr));
    viewci.image = this->image;
    chk(vkCreateImageView(vk.ldevice.handle, &viewci, nullptr, &this->view));
}

void BasicTexture::Destroy(FullDevice& vk){
    vkDestroyImageView(vk.ldevice.handle, this->view, nullptr);
    vmaDestroyImage(vk.vma.allocator, this->image, this->alloc);
}

VkFormat BasicTexture::supportedFormat(FullDevice& vk, std::vector<VkFormat>& formats, VkFormatFeatureFlags attachflag){
    VkFormatProperties2 properties = { .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
    for (VkFormat& format : formats) {
        vkGetPhysicalDeviceFormatProperties2(vk.ldevice.physical, format, &properties); // get format detail

        // is format supported?
        if (properties.formatProperties.optimalTilingFeatures & attachflag) {
            return format;
        }
    }

    return VkFormat{VK_FORMAT_UNDEFINED};
}

// DepthTexture

VmaAllocationCreateInfo DepthTexture::getAllocci(){
    return VmaAllocationCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
}

VkImageViewCreateInfo DepthTexture::getViewci(){
    return VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = this->imageci.format,
        .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
    };
};

void DepthTexture::Init(FullDevice& vk, glm::ivec2& size){
    std::vector<VkFormat> formats{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    VkFormat format = BasicTexture::supportedFormat(vk, formats, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    chk(format);

    // init the creation info
    this->imageci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent{.width = static_cast<uint32_t>(size.x), .height = static_cast<uint32_t>(size.y), .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    // actually create stuff
    auto allocci = this->getAllocci();
    auto viewci = this->getViewci();
    BasicTexture::createImageAndView(vk,this->imageci, allocci, viewci);
}

void DepthTexture::Resize(FullDevice& vk, glm::ivec2& size){
    // very similar to Init().
    this->Destroy(vk);
    this->imageci.extent = { .width = static_cast<uint32_t>(size.x), .height = static_cast<uint32_t>(size.y), .depth = 1 };
    auto allocci = getAllocci();
    auto viewci = getViewci();
    BasicTexture::createImageAndView(vk,this->imageci, allocci, viewci);
}

// ColorTexture

void ColorTexture::Init(FullDevice &vk, glm::ivec2& size){
    this->imageci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {
            .width  = static_cast<uint32_t>(size.x),
            .height = static_cast<uint32_t>(size.y),
            .depth  = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // if read from as well.
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // image is owned by one queue family.
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocci{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkImageViewCreateInfo viewci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageci.format,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = imageci.mipLevels, .layerCount = 1 }
    };

    BasicTexture::createImageAndView(vk, imageci, allocci, viewci);
}
