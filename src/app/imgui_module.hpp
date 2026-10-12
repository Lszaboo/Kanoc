#pragma once

#include "imgui.h"
#include <vulkan/vulkan_core.h>
#include "app/glfw_module.hpp"
#include "app/vk_module.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

class ImGuiModule{
    public:
        // fields
        ImGui_ImplVulkan_InitInfo initInfo = {};
        VkDescriptorPool pool = {VK_NULL_HANDLE};
        bool demo = true;

        // functions
        void Init(GLFWModule &glfw, VkModule &vk);
        void initDesriptorPool(VkModule &vk);

        void beginFrame();
        void updateFrame();
        void RecordRenderCommands(uint32_t &frameIndex, VkModule &vk);
        void recreateImGui(VkModule &vk);

        void Destroy(VkModule &vk);
};
