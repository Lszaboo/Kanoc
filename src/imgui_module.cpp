#include "kanoc/imgui_module.hpp"
#include "kanoc/vk_module.hpp"
#include "kanoc/chk.hpp"
#include "imgui.h"
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

void ImGuiModule::Init(GLFWModule &glfw, VkModule &vk, const std::string& workingPath){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsLight();

    initDesriptorPool(vk);

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForVulkan(glfw.window, true);
    initInfo.Instance = vk.vk.instance;
    initInfo.PhysicalDevice = vk.vk.ldevice.physical;
    initInfo.Device = vk.vk.ldevice.handle;
    initInfo.QueueFamily = vk.queue.idx;
    initInfo.Queue = vk.queue.handle;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = pool;
    initInfo.Allocator = nullptr;
    initInfo.MinImageCount = vk.ssc.swapchainci.minImageCount;
    initInfo.ImageCount = vk.ssc.count;
    initInfo.CheckVkResultFn = chk;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &vk.ssc.swapchainci.imageFormat,
        .depthAttachmentFormat = vk.sscdepth.imageci.format,
    };
    ImGui_ImplVulkan_Init(&initInfo);

    std::string textFontPath = workingPath + "/fonts/Roboto.ttf";
    textFont = io.Fonts->AddFontFromFileTTF(textFontPath.c_str(),18);

    std::string iconFontPath = workingPath + "/fonts/MaterialSymbolsRounded.ttf";
    iconFont = io.Fonts->AddFontFromFileTTF(iconFontPath.c_str(),26);
}

void ImGuiModule::initDesriptorPool(VkModule &vk){
    VkDescriptorPoolSize pool_sizes[] = {
        // type                             count
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE },
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 0;
    for (VkDescriptorPoolSize& pool_size : pool_sizes)
    pool_info.maxSets += pool_size.descriptorCount;
    pool_info.poolSizeCount = (uint32_t)IM_COUNTOF(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    chk(vkCreateDescriptorPool(vk.vk.ldevice.handle, &pool_info, nullptr, &pool));
}

void ImGuiModule::beginFrame(){
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::PushFont(textFont);
}

void ImGuiModule::endFrame(){
    ImGui::PopFont();
}

void ImGuiModule::RecordRenderCommands(uint32_t &frameIndex, VkModule &vk){
    ImGui::Render();
    ImDrawData *data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(data, vk.commandBuffers[frameIndex]);
}

void ImGuiModule::Destroy(VkModule &vk){
    vkDeviceWaitIdle(vk.vk.ldevice.handle);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(vk.vk.ldevice.handle, pool, nullptr);
}

void ImGuiModule::recreateImGui(VkModule &vk){
    ImGui_ImplVulkan_SetMinImageCount(vk.ssc.swapchainci.minImageCount);
}
