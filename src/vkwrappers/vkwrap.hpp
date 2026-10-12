#pragma once

#include "vkwrappers/devices.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <vector>

struct SurfaceSwapChain{
    VkSurfaceKHR surface = {VK_NULL_HANDLE};
    VkSwapchainKHR swapchain = {VK_NULL_HANDLE};
    VkSwapchainCreateInfoKHR swapchainci = {};
    uint32_t count; // number of swapchain images
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
};

struct GraphicsQueue{
    VkQueue handle = {VK_NULL_HANDLE};
    uint32_t idx = 0; // QueueFamilyIndex
};
