#pragma once

#include "kanoc/devices.hpp"
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

        void Destroy(FullDevice& vk);
};
