#pragma once
#include <cstdint>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <vector>
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

#include <array>

#include "kanoc/namedresource.hpp"
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
        Named<SamplerTexture*> idtex; // Index of objects is rendered into this texture.
        std::array<Named<StagingBuffer*>, VkModule::maxFramesInFlight> idbufs; // Downloads idtex pixels from GPU, and can be accessed via Host (CPU).
        std::array<Named<VkFence>, VkModule::maxFramesInFlight> idfences; // Host (CPU) must wait on these fences for idtex Download into idbufs[i].

        Named<VkShaderModule> combinerShader;
        Named<VkDescriptorSetLayout> combinerDescriptorLayout;
        Named<Descriptors*> combinerDescriptors;
        Named<GraphicsPipeline*> combinerPipeline;

        void Init(Modules* m, ResourceOwners& resources);
        void Resize(VkModule& vkmod);

        void waitIdTexComplete(Modules* m);
        uint32_t readIdTex(glm::ivec2 windowSize, glm::ivec2 mouseCoord, uint32_t frameIndex);

        void BeginRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& nextsscimage, SurfaceSwapChain& ssc, DepthTexture& sscdepth, uint32_t& frameIndex);
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
            uint32_t id = 0;
        };

        Named<ObjModel*> model;
        std::array<Named<SamplerTexture*>, 1> textures = {};

        Named<VkShaderModule> shader;
        Named<GraphicsPipeline*> pipeline;

        Named<Descriptors*> descriptorpool;
        Named<VkDescriptorSetLayout> descriptorlayout;

        // uniforms per frames-in-flight, because the CPU will upload while a frame is processed.
        std::array<Named<ShaderBuffer*>, VkModule::maxFramesInFlight> uniforms;

        void Init(Modules* m, IndexVK& indexVk, ResourceOwners& resources);
        void BeginRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, SamplerTexture& coltex, DepthTexture& coldepth, SamplerTexture& idtex);
        void RecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& frameIndex);
        void EndRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, SamplerTexture& coltex, SamplerTexture& idtex);

    protected:
        void InitDescriptors(VkModule& vkmod, ResourceOwners& resources);
        void InitPipeline(VkModule& vkmod, IndexVK& indexVk, ResourceOwners& resources);
};

class GuiVk{
    public:
        bool shown = true;
        bool plotEveryFrame = false;
        bool readUnderMouse = false;
        int sampleStep = 10;
        std::vector<uint32_t> ids;
        glm::ivec2 iddim;

        void update(Modules* m, IndexVK& indexVk, glm::vec2& rawCursorPos, glm::ivec2& correctedCursorPos, bool guiCapturedCursor, uint32_t& id, std::vector<int>& pressedKeys, std::vector<int>& releasedKeys);
        void fillIds(VkModule& vkmod, StagingBuffer& idbuf);
};

class App{
    public:
        // functions
        void Init(Modules* modules);
        void Destroy();
        void Play();

        Modules* modules();
        void cursorPosChanged(glm::vec2 rawCursorPos, glm::ivec2 correctedCursorPos, bool guiCapturedCursor);
        void keyPressed(int key);
        void keyReleased(int key);

    protected:
        // Kanoc harness
        Modules* m;

        // Resources
        ResourceOwners resources;

        // Indexed rendering resources
        IndexVK indexVk;

        // Duck rendering resources
        DuckVk duckVk;

        // Gui rendering resources
        GuiVk guiVk;

        // Command recording helpers
        CommandRecorders cmds;

        // functions
        void printApiData();

        void updateDeltaTime();
        void updateShaderData();
        void pollEvents();
        void updateAppLogic();

        double time = 0.0;
        double oldTime = 0.0;
        double deltaTime = 0.0;

        uint32_t imageIndex = 0;
        bool updateSwapchain = false;
        DuckVk::Uniforms shaderData = {};
        glm::vec3 camPos = { 0.0f, 0.0f, -6.0f };
        glm::vec3 objectRotations[3] = {};
        glm::vec2 rawCursorPos = {};
        glm::ivec2 correctedCursorPos = {};
        bool guiCapturedCursor = false;
        std::vector<int> pressedKeys;
        std::vector<int> releasedKeys;

};
