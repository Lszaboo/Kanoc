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

#include <vk_mem_alloc.h>

#include <tiny_obj_loader.h>

#include <ktx.h>
#include <ktxvulkan.h>

#include <array>

#include "kanoc/namedresource.hpp"
#include "kanoc/harness.hpp"
#include "kanoc/resources.hpp"
#include "kanoc/buffers.hpp"
#include "kanoc/descriptors.hpp"
#include "kanoc/pipelines.hpp"
#include "kanoc/textures.hpp"
#include "kanoc/vk_module.hpp"
#include "kanoc/cmdrecorders.hpp"
#include "kanoc/indexvk.hpp"

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

class AppGui{
    public:
        bool shown = true;
        bool plotEveryFrame = false;
        bool readUnderMouse = false;
        int sampleStep = 10;
        std::vector<uint32_t> ids;
        glm::ivec2 iddim;

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
        AppGui gui;

        // Command recording helpers
        CommandRecorders cmds;

        // functions
        void printApiData();

        void updateShaderData();
        void pollEvents();
        void updateAppGui();
        void updateAppLogic();

        bool updateSwapchain = false;
        DuckVk::Uniforms shaderData = {};

        glm::vec3 camPos = { 0.0f, 0.0f, -6.0f };
        std::array<glm::vec3, 3> objectRotations = {};

        glm::vec2 rawCursorPos = {};
        glm::ivec2 correctedCursorPos = {};
        bool guiCapturedCursor = false;

        std::vector<int> keys;
        std::vector<double> keyTimes;
        std::vector<bool> keysPressedOrReleased;
};
