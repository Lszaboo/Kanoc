#pragma once

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <tiny_obj_loader.h>

#include <ktx.h>
#include <ktxvulkan.h>

#include "namedresource.hpp"
#include "kanoc/harness.hpp"
#include "resources.hpp"
#include "buffers.hpp"
#include "descriptors.hpp"
#include "pipelines.hpp"
#include "textures.hpp"
#include "vk_module.hpp"
#include "cmdrecorders.hpp"

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
