#include "kanoc/surfaceswapchain.hpp"

void SurfaceSwapChain::Destroy(FullDevice& vk){
    for (auto& imageAcquiredSemaphore : imageAcquiredSemaphores) {
		vkDestroySemaphore(vk.ldevice.handle, imageAcquiredSemaphore, nullptr);
	}

    for (auto& renderCompleteSemaphore : imageRenderCompleteSemaphores) {
		vkDestroySemaphore(vk.ldevice.handle, renderCompleteSemaphore, nullptr);
	}

    for (auto & view : views) {
		vkDestroyImageView(vk.ldevice.handle, view, nullptr);
	}

	depth.Destroy(vk);

	vkDestroySwapchainKHR(vk.ldevice.handle, swapchain, nullptr);

	vkDestroySurfaceKHR(vk.instance, surface, nullptr);
}
