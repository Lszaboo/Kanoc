#pragma once
#include <cstdint>

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "imgui.h"

#include <vk_mem_alloc.h>
#include <tiny_obj_loader.h>
#include <ktx.h>
#include <ktxvulkan.h>

#include "kanoc/namedresource.hpp"

#include <array>

#include "kanoc/harness.hpp"
#include "kanoc/chk.hpp"
#include "kanoc/resources.hpp"
#include "kanoc/buffers.hpp"
#include "kanoc/descriptors.hpp"
#include "kanoc/devices.hpp"
#include "kanoc/imagebarriers.hpp"
#include "kanoc/pipelines.hpp"
#include "kanoc/shaders.hpp"
#include "kanoc/textures.hpp"
#include "kanoc/uniforms.hpp"
#include "kanoc/vkwrap.hpp"
#include "kanoc/glfw_module.hpp"
#include "kanoc/vk_module.hpp"
#include "kanoc/imgui_module.hpp"

struct CommandRecorders{
    VkCommandBuffer cb = {VK_NULL_HANDLE};
    ImageBarrierCommander ibc{};
    DynamicRenderingCommander drc{};
};

class IndexVK{
    public:
        Named<DepthTexture*> depth; // Depth for the scene.
        Named<SamplerTexture*> coltex; // Color Texture of the scene.
        Named<IndexTexture*> idtex; // Index of objects is rendered into this texture.

        Named<VkShaderModule> combinerShader;
        Named<VkDescriptorSetLayout> combinerDescriptorLayout;
        Named<Descriptors*> combinerDescriptors;
        Named<GraphicsPipeline*> combinerPipeline;

        void Init(VkModule& vkmod, ResourceOwners& resources);
        void Resize(VkModule& vkmod);
        void BeginRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& nextsscimage, SurfaceSwapChain& ssc, DepthTexture& sscdepth);
        void RecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& nextsscimage, SurfaceSwapChain& ssc, DepthTexture& sscdepth);
        void EndRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& nextsscimage, SurfaceSwapChain& ssc, DepthTexture& sscdepth);


    protected:
        void InitDescriptors(VkModule& vkmod, ResourceOwners& resources);
        void InitCombinerPipeline(VkModule& vkmod, ResourceOwners& resources);
};

class DuckVk{
    public:
        struct Uniforms {
            glm::mat4 projection;
            glm::mat4 view;
            glm::mat4 model[3];
            glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f };
            uint32_t selected{1};
        };

        Named<ObjModel*> model;
        std::array<Named<SamplerTexture*>, 1> textures = {};

        Named<VkShaderModule> shader;
        Named<GraphicsPipeline*> pipeline;

        Named<Descriptors*> descriptorpool;
        Named<VkDescriptorSetLayout> descriptorlayout;

        // uniforms per frames-in-flight, because the CPU will upload while a frame is processed.
        std::array<Named<ShaderDataBuffer*>, VkModule::maxFramesInFlight> uniforms;

        void Init(VkModule& vkmod, IndexVK& indexVk, ResourceOwners& resources);
        void BeginRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, SamplerTexture& coltex, DepthTexture& coldepth);
        void RecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& frameIndex);
        void EndRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, SamplerTexture& coltex, DepthTexture& coldepth);

    protected:
        void InitDescriptors(VkModule& vkmod, ResourceOwners& resources);
        void InitPipeline(VkModule& vkmod, IndexVK& indexVk, ResourceOwners& resources);
};

class App{
    public:
        // functions
        void Init(Harness<App>::Modules& m);
        void Destroy(Harness<App>::Modules& m);
        void Play(Harness<App>::Modules& m);

    protected:
        // Resources
        ResourceOwners resources;

        // Indexed rendering resources
        IndexVK indexVk;

        // Duck rendering resources
        DuckVk duckVk;

        // Command recording helpers
        CommandRecorders cmds;

        // functions
        void printApiData(Harness<App>::Modules& m);

        void updateDeltaTime(Harness<App>::Modules& m);
        void updateShaderData(Harness<App>::Modules& m);
        void updateFrameIndex(Harness<App>::Modules& m);
        void pollEvents(Harness<App>::Modules& m);


        double oldTime = 0.0;
        double deltaTime = 0.0;
        uint32_t frameIndex = 0;
        uint32_t imageIndex = 0;
        bool updateSwapchain = false;
        DuckVk::Uniforms shaderData = {};
        glm::vec3 camPos = { 0.0f, 0.0f, -6.0f };
        glm::vec3 objectRotations[3] = {};
        double lastMouseX = 0.0;
        double lastMouseY = 0.0;
};
