#pragma once
#include "kanoc/cmd_helpers.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vulkan/vulkan_core.h>

static inline void chk(VkResult result){
    if (result != VK_SUCCESS) {
        std::cout<< "VK error occured:\n" << result << std::endl;
        exit(result);
    }
}

static inline void glfw_chk(int val){
    if (val==GLFW_FALSE) {
        std::cout<< "VK error occured with GLFW!"<< std::endl;
        exit(1);
    }
}

static inline void chk_obj(bool val){
    if (!val) {
        std::cout<< "OBJ error occured!"<< std::endl;
        exit(1);
    }
}

static inline void chkSwapchain(VkResult result, bool& updateSwapchain) {
    if (result < VK_SUCCESS) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			updateSwapchain = true;
			return;
		}
		std::cerr << "Vulkan call returned an error (" << result << ")\n";
		exit(result);
	}
    updateSwapchain = false;
}

static inline void chk(VkFormat result){
    if (result == VK_FORMAT_UNDEFINED){
        std::cout<< "VK_FORMAT error occured!" << std::endl;
        exit(1);
    }
}
