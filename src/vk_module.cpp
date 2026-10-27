#include "kanoc/vk_module.hpp"
#include "kanoc/glfw_module.hpp"
#include "kanoc/chk.hpp"
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_uint_sized.hpp>
#include <glm/ext/vector_float3.hpp>
#include <ostream>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <string>
#include <iostream>

#include "kanoc/cmdhelpers.hpp"
#include "kanoc/debug.hpp"
#include "kanoc/dynamicrendering.hpp"
#include "kanoc/imagebarriers.hpp"
#include "kanoc/textures.hpp"

void VkModule::Init(GLFWModule &glfw){
    glfw.fillRequiredExtensions(vk.extensions);

    initInstance(init);
    initPDevices(init);
    initQueues(init);
    initDevice(init);
    initVMA(init);
    initSurface(init, glfw);
    initSwapchain(init, glfw);

    sscdepth.Init(vk, windowSize);

    initSynchronization(init);
    initCommandBuffer(init);

    slang.Init();
}

void VkModule::Destroy(){
    WaitForIdle();
	for (auto i = 0; i < maxFramesInFlight; i++) {
		vkDestroyFence(vk.ldevice.handle, fences[i], nullptr);
		vkDestroySemaphore(vk.ldevice.handle, imageAcquiredSemaphores[i], nullptr);
	}
	for (auto i = 0; i < renderCompleteSemaphores.size(); i++) {
		vkDestroySemaphore(vk.ldevice.handle, renderCompleteSemaphores[i], nullptr);
	}

	sscdepth.Destroy(vk);

	for (auto i = 0; i < ssc.views.size(); i++) {
		vkDestroyImageView(vk.ldevice.handle, ssc.views[i], nullptr);
	}

	vkDestroySwapchainKHR(vk.ldevice.handle, ssc.swapchain, nullptr);
	vkDestroySurfaceKHR(vk.instance, ssc.surface, nullptr);
	vkDestroyCommandPool(vk.ldevice.handle, commandPool, nullptr);

	vmaDestroyAllocator(vk.vma.allocator); //

	vkDestroyDevice(vk.ldevice.handle, nullptr);
	vkDestroyInstance(vk.instance, nullptr);
}

void VkModule::initInstance(InitStack &init){
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, // Tells whats's the type of the structure.
        .pApplicationName = "VulkanApp",
        .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = vk.extensions.count,
        .ppEnabledExtensionNames = vk.extensions.items.data()
    };

    chk(vkCreateInstance(&instanceCI, nullptr, &vk.instance));
}

void VkModule::initPDevices(InitStack &init){
    uint32_t deviceCount{0};
    chk(vkEnumeratePhysicalDevices(vk.instance, &deviceCount, nullptr)); // get count

    init.pdevices.resize(deviceCount);
    chk(vkEnumeratePhysicalDevices(vk.instance, &deviceCount, init.pdevices.data())); // put it into vector

    if (init.pdeviceID >= deviceCount) {
        init.pdeviceID = 0;
    }

    printDevices(init.pdevices, deviceCount);
}

void VkModule::printDevices(std::vector<VkPhysicalDevice> &pdevices, uint32_t &pdeviceCount){
    VkPhysicalDeviceProperties2 deviceInfo {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };

    std::cout << "----- Devices: -----" << CMD_ENDL;
    for (uint32_t dIdx = 0; dIdx < pdeviceCount; dIdx++) {
        vkGetPhysicalDeviceProperties2(pdevices[dIdx], &deviceInfo);
        std::cout
            << "\tDeviceID: " << deviceInfo.properties.deviceID << "\n"
            << "\t\tName:" << deviceInfo.properties.deviceName << "\n"
            << "\t\tVendorID: " << deviceInfo.properties.vendorID << "\n"
            << "\t\tDeviceType: " << deviceInfo.properties.deviceType << "\n"
            << "\t\tDriver Version: " << deviceInfo.properties.driverVersion << "\n"
            << "\t\tAPI Version: " << deviceInfo.properties.apiVersion <<
        CMD_ENDL;
    }
}

void VkModule::initQueues(InitStack &init){
    vk.ldevice.physical = init.pdevices[0];

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk.ldevice.physical, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vk.ldevice.physical, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i<queueFamilyCount ; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue.idx = i;
            break;
        }
    }

    glfw_chk(glfwGetPhysicalDevicePresentationSupport(vk.instance, vk.ldevice.physical, queue.idx));

    init.queuePriority = 1.0;
    init.queueCI = VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue.idx,
        .queueCount = 1,
        .pQueuePriorities = &init.queuePriority
    };
}

void VkModule::initDevice(InitStack &init){
    const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkPhysicalDeviceVulkan12Features vk12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true
    };

    VkPhysicalDeviceVulkan13Features vk13Featuers{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &vk12Features,
        .synchronization2 = true,
        .dynamicRendering = true
    };

    VkPhysicalDeviceFeatures vk10Features{
        .independentBlend = VK_TRUE, // Enable independent blend for IndexVK.
        .samplerAnisotropy = VK_TRUE
    };

    VkDeviceCreateInfo deviceCInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &vk13Featuers,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &init.queueCI,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &vk10Features
    };

    chk(vkCreateDevice(vk.ldevice.physical, &deviceCInfo, nullptr, &vk.ldevice.handle));
    vkGetDeviceQueue(vk.ldevice.handle, queue.idx, 0, &queue.handle);
}

void VkModule::initVMA(InitStack &init){
    VmaVulkanFunctions vmaVkFunctions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage
    };

    VmaAllocatorCreateInfo allocatorCI{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = vk.ldevice.physical,
        .device = vk.ldevice.handle,
        .pVulkanFunctions = &vmaVkFunctions,
        .instance = vk.instance
    };

    chk(vmaCreateAllocator(&allocatorCI, &vk.vma.allocator));
}

void VkModule::initSurface(InitStack &init, GLFWModule &glfw){
    chk(glfwCreateWindowSurface(vk.instance, glfw.window, nullptr, &ssc.surface));
    glfwGetFramebufferSize(glfw.window, &windowSize.x, &windowSize.y);
}

void VkModule::initSwapchain(InitStack &init, GLFWModule &glfw){
    VkSurfaceCapabilitiesKHR surfaceCaps = {};
    chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.ldevice.physical, ssc.surface, &surfaceCaps));
    VkExtent2D swapchainExtent{ surfaceCaps.currentExtent };
    if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
        swapchainExtent = { .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) };
    }

    ssc.swapchainci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = ssc.surface,
        .minImageCount = surfaceCaps.minImageCount,
        .imageFormat = VK_FORMAT_B8G8R8A8_SRGB ,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent{.width = swapchainExtent.width, .height = swapchainExtent.height },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR
    };
    chk(vkCreateSwapchainKHR(vk.ldevice.handle, &ssc.swapchainci, nullptr, &ssc.swapchain));

    chk(vkGetSwapchainImagesKHR(vk.ldevice.handle, ssc.swapchain, &ssc.count, nullptr));
    ssc.images.resize(ssc.count);
    chk(vkGetSwapchainImagesKHR(vk.ldevice.handle, ssc.swapchain, &ssc.count, ssc.images.data()));

    ssc.views.resize(ssc.count);
    for (auto i = 0; i < ssc.count; i++) {
		VkImageViewCreateInfo viewCI{
		    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = ssc.images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = ssc.swapchainci.imageFormat,
			.subresourceRange{  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			                    .levelCount = 1,
								.layerCount = 1
			                }
		};
		chk(vkCreateImageView(vk.ldevice.handle, &viewCI, nullptr, &ssc.views[i]));
	}
}

void VkModule::initSynchronization(InitStack &init){
    init.semaphoreCI = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    for (auto i = 0; i < maxFramesInFlight; i++) {
        chk(vkCreateFence(vk.ldevice.handle, &fenceCI, nullptr, &fences[i]));
        chk(vkCreateSemaphore(vk.ldevice.handle, &init.semaphoreCI, nullptr, &imageAcquiredSemaphores[i]));
    }
    renderCompleteSemaphores.resize(ssc.images.size());
    for (auto& semaphore : renderCompleteSemaphores) {
        chk(vkCreateSemaphore(vk.ldevice.handle, &init.semaphoreCI, nullptr, &semaphore));
    }
}

void VkModule::initCommandBuffer(InitStack &init){
    // cb pool
    VkCommandPoolCreateInfo commandPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue.idx
    };
    chk(vkCreateCommandPool(vk.ldevice.handle, &commandPoolCI, nullptr, &commandPool));

    // cb-s
    VkCommandBufferAllocateInfo cbAllocCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .commandBufferCount = maxFramesInFlight
    };
    chk(vkAllocateCommandBuffers(vk.ldevice.handle, &cbAllocCI, commandBuffers.data()));
}

uint32_t VkModule::nextFrameIdx(uint32_t fidx){
    return (fidx + 1) % VkModule::maxFramesInFlight;
}

uint32_t VkModule::previousFrameIdx(uint32_t fidx){
    return (fidx-1)==-1 ? (VkModule::maxFramesInFlight-1) : (fidx-1);
}

void VkModule::updateFrameIdx(){
    frameIdx = VkModule::nextFrameIdx(frameIdx);
}

void VkModule::updateFence(){
    // wait for a fence indicating rendering finished to become "signaled"
    chk(vkWaitForFences(vk.ldevice.handle, 1, &fences[frameIdx], true, UINT64_MAX));
    // fence becomes "unsignaled"
    chk(vkResetFences(vk.ldevice.handle, 1, &fences[frameIdx]));
}

/*
 *  The function gets the index of the next ssc image, and assigns the
 *  current's frames semaphore to wait on the next ssc image acquisition.
 *  - The imagindex parameter will index the next ssc image.
 *  - The return of the function is true if the swapchain needs recreation.
 */
bool VkModule::nextSSCImage(uint32_t &imageIndex){
    // give index of next swapchain image
    // also signals semaphore when image is ready
    /*
     * The good news is there’s a simple way to guarantee that the presentation operation has finished,
     * though less direct than an explicit wait.
     * Acquiring the image index from vkAcquireNextImageKHR and then waiting on its semaphore or fence guarantees that the previous
     * presentation operation that used the just-acquired image index has completed,
     * which includes the wait on VkPresentInfoKHR::pWaitSemaphores,
     * so the corresponding semaphores can be reused.
     *
     * (...)
     *
     * A common mistake is applying a buffering scheme based on the number of frames in flight (such as double or triple buffering)
     * to present wait semaphores. Often, to synchronize with in-flight frames,
     * the application uses a vkQueueSubmit fence. By waiting on that fence,
     * we know that the corresponding command buffers and other frame resources are no longer in use.
     * However, the Vulkan specification does not guarantee that waiting on a vkQueueSubmit fence
     * also synchronizes presentation operations.
     * The reuse of presentation resources should rely on vkAcquireNextImageKHR
     * or additional vk.extensions (will be mentioned at the end of this article), rather than on vkQueueSubmit fences.
     */
    VkResult result = vkAcquireNextImageKHR(vk.ldevice.handle, ssc.swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIdx], VK_NULL_HANDLE, &imageIndex);

    if (result < VK_SUCCESS) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return true;
		}
		std::cerr << "Vulkan ssc call returned an error (" << result << ")\n";
		exit(result);
	}
    return false;
}

void VkModule::BeginCBRecordingToSSC(){
    auto cb = commandBuffers[frameIdx];

    // reset buffer
    chk(vkResetCommandBuffer(cb, 0));

    // begin command recording
    VkCommandBufferBeginInfo cbBI {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    chk(vkBeginCommandBuffer(cb, &cbBI));
}

void VkModule::EndCBRecordingToSSC(){
    vkEndCommandBuffer(commandBuffers[frameIdx]);
}

void VkModule::updateSubmitCommandBuffer(uint32_t& imageIndex){
    auto cb = commandBuffers[frameIdx];

    // wait before color attachment for HKR image semaphore to be available
    // this is so it can be attached
    // vkAcquireNextImageKHR signals this
    VkSemaphoreSubmitInfo waitSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = imageAcquiredSemaphores[frameIdx],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    VkCommandBufferSubmitInfo commandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cb
    };

    // signal this semaphore, when color attachmet is completed
    // imageIndex is the current KHR image
    VkSemaphoreSubmitInfo signalSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = renderCompleteSemaphores[imageIndex],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    VkSubmitInfo2 submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalSemaphoreInfo,
    };
    chk(vkQueueSubmit2(queue.handle, 1, &submitInfo, fences[frameIdx]));
}

bool VkModule::queuePresentImage(uint32_t& imageIndex){
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderCompleteSemaphores[imageIndex], // Wait for semaphore, indicating rendering to image is complete.
        .swapchainCount = 1,
        .pSwapchains = &ssc.swapchain,
        .pImageIndices = &imageIndex
    };

    VkResult result = vkQueuePresentKHR(queue.handle, &presentInfo);
    if (result < VK_SUCCESS) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return true;
		}
		std::cerr << "Vulkan call returned an error (" << result << ")\n";
		exit(result);
	}else{
	    return false;
	}
}

void VkModule::recreateSwapchains(GLFWModule &glfw){
    // swapchain
    VkSurfaceCapabilitiesKHR surfaceCaps = {};
    chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(init.pdevices[init.pdeviceID], ssc.surface, &surfaceCaps));

    VkExtent2D extent = {surfaceCaps.currentExtent};
    if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
        extent = { .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) };
    }

    ssc.swapchainci.oldSwapchain = ssc.swapchain;
    ssc.swapchainci.imageExtent = extent;
    chk(vkCreateSwapchainKHR(vk.ldevice.handle, &ssc.swapchainci, nullptr, &ssc.swapchain));

    for (auto i = 0; i < ssc.count; i++) {
        vkDestroyImageView(vk.ldevice.handle, ssc.views[i], nullptr);
    }
    chk(vkGetSwapchainImagesKHR(vk.ldevice.handle, ssc.swapchain, &ssc.count, nullptr));
    ssc.images.resize(ssc.count);
    chk(vkGetSwapchainImagesKHR(vk.ldevice.handle, ssc.swapchain, &ssc.count, ssc.images.data()));
    ssc.views.resize(ssc.count);
    for (auto i = 0; i < ssc.count; i++) {
        VkImageViewCreateInfo viewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = ssc.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = ssc.swapchainci.imageFormat,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}
        };
        chk(vkCreateImageView(vk.ldevice.handle, &viewCI, nullptr, &ssc.views[i]));
    }
    // ssc available semaphores.
    for (auto& semaphore : imageAcquiredSemaphores) {
        vkDestroySemaphore(vk.ldevice.handle, semaphore, nullptr);
    }
    for (auto& semaphore : imageAcquiredSemaphores) {
        chk(vkCreateSemaphore(vk.ldevice.handle, &init.semaphoreCI, nullptr, &semaphore));
    }

    // ssc complete semaphores.
    for (auto& semaphore : renderCompleteSemaphores) {
        vkDestroySemaphore(vk.ldevice.handle, semaphore, nullptr);
    }
    renderCompleteSemaphores.resize(ssc.count);
    for (auto& semaphore : renderCompleteSemaphores) {
        chk(vkCreateSemaphore(vk.ldevice.handle, &init.semaphoreCI, nullptr, &semaphore));
    }

    vkDestroySwapchainKHR(vk.ldevice.handle, ssc.swapchainci.oldSwapchain, nullptr);

    // depth
    sscdepth.Resize(vk, windowSize);
}

void VkModule::WaitForIdle(){
    chk(vkDeviceWaitIdle(vk.ldevice.handle));
}
