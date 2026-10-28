#pragma once

#include <cstdint>

#include <glm/ext/scalar_uint_sized.hpp>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vk_mem_alloc.h>
#include <tiny_obj_loader.h>
#include <ktx.h>
#include <ktxvulkan.h>

#include <vector>
#include <array>

#include "kanoc/devices.hpp"
#include "kanoc/surfaceswapchain.hpp"
#include "kanoc/dynamicrendering.hpp"
#include "kanoc/imagebarriers.hpp"
#include "kanoc/textures.hpp"
#include "kanoc/shaders.hpp"
#include "kanoc/vkwrap.hpp"

#include "kanoc/glfw_module.hpp"

struct InitStack{
    int pdeviceID = 0;
    std::vector<VkPhysicalDevice> pdevices = {};
    float queuePriority = 1.0;
    VkDeviceQueueCreateInfo queueCI = {};
    VkSemaphoreCreateInfo semaphoreCI = {};
};

class VkModule{
    public:

        // fields
        FullDevice vk;
        InitStack init;
        glm::ivec2 windowDims;

        SurfaceSwapChain ssc; // Swapchain of the window.
        DepthTexture sscdepth; // Depth for ssc rendering.

        VkCommandPool commandPool = { VK_NULL_HANDLE }; // command buffers come from this.
        GraphicsQueue queue; // Commands buffers are sent through this queue.

        static const uint32_t maxFramesInFlight = 2;
        static uint32_t nextFrameIdx(uint32_t fidx);
        static uint32_t previousFrameIdx(uint32_t fidx);

        uint32_t frameIdx = 0;
        void updateFrameIdx();
        std::array<VkCommandBuffer, maxFramesInFlight> frameCommandBuffers; // Commands are recorded into this buffer.
        std::array<VkFence, maxFramesInFlight> frameFinishedFences; // Is rendering finished for this frame-in-flight?

        std::array<VkSemaphore, maxFramesInFlight> sscImageAcquiredSemaphores; // signal this, when for this frame KHR image is available
        std::vector<VkSemaphore> sscImageRenderCompleteSemaphores; // signal this, when this KHR image is complete (size() == KHR image amount)

        SlangCompiler slang;

        // functions
        void Init(GLFWModule &glfw);
        void Destroy();
        void WaitForIdle();

        void initInstance(InitStack &init);
        void initPDevices(InitStack &init);
        void initQueues(InitStack &init);
        void initDevice(InitStack &init);
        void initVMA(InitStack &init);
        void initSurface(InitStack &init, GLFWModule &glfw);
        void initSwapchain(InitStack &init, GLFWModule &glfw);

        //void initUniforms(InitStack &init);
        void initSynchronization(InitStack &init);
        void initCommandBuffer(InitStack &init);

        void printDevices(std::vector<VkPhysicalDevice> &pdevices, uint32_t &pdeviceCount);

        void updateFence();
        bool nextSSCImage(uint32_t &imageIndex);

        void BeginCBRecordingToSSC();
        void EndCBRecordingToSSC();

        void updateSubmitCommandBuffer(uint32_t &imageIndex);
        bool queuePresentImage(uint32_t &imageIndex);

        void recreateSwapchains(GLFWModule &glfw);
};
