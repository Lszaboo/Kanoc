#include "kanoc/surfaceswapchain.hpp"

void SurfaceSwapChain::Destroy(FullDevice& vk){
    for (auto i = 0; i < views.size(); i++) {
		vkDestroyImageView(vk.ldevice.handle, views[i], nullptr);
	}
	vkDestroySwapchainKHR(vk.ldevice.handle, swapchain, nullptr);
	vkDestroySurfaceKHR(vk.instance, surface, nullptr);
}
