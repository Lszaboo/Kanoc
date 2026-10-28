#pragma once

#include "kanoc/devices.hpp"
#include "kanoc/textures.hpp"
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <vector>

class SurfaceSwapChain{
    public:
        VkSurfaceKHR surface = {VK_NULL_HANDLE};
        VkSwapchainKHR swapchain = {VK_NULL_HANDLE};
        VkSwapchainCreateInfoKHR swapchainci = {};

        uint32_t count; // number of swapchain images

        std::vector<VkImage> images;
        std::vector<VkImageView> views;
        DepthTexture depth; // Depth for ssc rendering.

        std::vector<VkSemaphore> imageAcquiredSemaphores; // Signal this, when for this frame KHR image is available (size() == Max frames in flight).
        std::vector<VkSemaphore> imageRenderCompleteSemaphores; // Signal this, when this KHR image is complete (size() == KHR image amount).

        void Destroy(FullDevice& vk);
};
