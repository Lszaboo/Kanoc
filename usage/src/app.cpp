#include "app.hpp"
#include "imgui.h"
#include "implot.h"
#include "kanoc/cmdhelpers.hpp"
#include "kanoc/harness.hpp"
#include "kanoc/vk_module.hpp"
#include "kanoc/chk.hpp"
#include "kanoc/debug.hpp"
#include "kanoc/resources.hpp"
#include "kanoc/buffers.hpp"
#include "kanoc/cmdbufs.hpp"
#include "kanoc/descriptors.hpp"
#include "kanoc/devices.hpp"
#include "kanoc/imagebarriers.hpp"
#include "kanoc/pipelines.hpp"
#include "kanoc/textures.hpp"
#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <ostream>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <string>
#include <iostream>

void IndexVK::Init(VkModule& vkmod, ResourceOwners& resources){
    auto depth_texture = DepthTexture();
    depth_texture.Init(vkmod.vk, vkmod.windowSize);
    depth = resources.depthtextures.Add("Index_Depth_Texture", std::move(depth_texture));

    auto color_texture = SamplerTexture();
    color_texture.InitColorReadWrite(vkmod.vk, vkmod.windowSize);
    coltex = resources.samplertextures.Add("Index_Color_texture", std::move(color_texture));

    OneTimeCB cb;
    ImageBarrierCommander ibc;
    ibc.Add({
        .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .image = coltex.resource->image,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } //{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 0, .layerCount = 1 }
    });
    cb.Begin(vkmod.vk, vkmod.commandPool);
    ibc.RecordAndClearCommands(cb.handle);
    cb.End(vkmod.vk, vkmod.queue);

    auto index_texture = SamplerTexture();
    index_texture.InitIDReadWrite(vkmod.vk, vkmod.windowSize);
    idtex = resources.samplertextures.Add("Index_Index_Texture", std::move(index_texture));

    uint32_t idtexsize = static_cast<uint32_t>(vkmod.windowSize.x * vkmod.windowSize.y) * sizeof(uint32_t);
    for (int i = 0; i < VkModule::maxFramesInFlight; i++) {
        std::string name = "Index_Index_Buffer" + std::to_string(i);
        idbufs[i] = resources.stagingbuffers.Add(name, StagingBuffer::BuildForDownload(vkmod.vk, idtexsize));
        name = "Index_Index_Fence" + std::to_string(i);
        idfences[i] = resources.fences.Add(name, BuildFence(vkmod.vk, {}));
    }

    VkShaderModule combiner_shader = {VK_NULL_HANDLE};
    vkmod.slang.LoadAndCompile(vkmod.vk, "shaders/combiner.slang", combiner_shader);
    combinerShader = resources.shaders.Add("Index_Combiner_Shader", std::move(combiner_shader));

    InitDescriptors(vkmod, resources);

    InitCombinerPipeline(vkmod, resources);

}

void IndexVK::InitDescriptors(VkModule& vkmod, ResourceOwners& resources){
    DescriptorLayoutBuilder layoutBuilder{};
    layoutBuilder.Add(
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        },
        0 // No flags needed.
    );
    layoutBuilder.Add(
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        },
        0 // No flags needed.
    );
    combinerDescriptorLayout = resources.descriptorlayouts.Add("Index_Combiner_Descriptor_Layout", layoutBuilder.Build(vkmod.vk));

    DescriptorPoolBuilder poolbuilder{};
    poolbuilder.Add(
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 2
        }
    );
    combinerDescriptors = resources.descriptorpools.Add("Index_Combiner_Descriptors", {.pool = poolbuilder.Build(vkmod.vk, 1)});

    DescriptorSetAllocator setAllocator{};
    setAllocator.Add(
        combinerDescriptorLayout.resource,
        1 // Since it is not a Variable sized layout, Vulkan doesn't really care.
    );
    setAllocator.Allocate(vkmod.vk, *combinerDescriptors.resource);

    // Upload descriptor set data.
    std::vector<VkDescriptorImageInfo> imageinfos = {
        {
            .sampler = coltex.resource->sampler,
            .imageView = coltex.resource->view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {
            .sampler = idtex.resource->sampler,
            .imageView = idtex.resource->view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
    };
    VkWriteDescriptorSet writeDescSet1{
        .dstSet = combinerDescriptors.resource->sets[0],
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Conform to the type at binding.
        .pImageInfo = &imageinfos[0]
    };
    combinerDescriptors.resource->WriteSet(vkmod.vk, writeDescSet1);
    VkWriteDescriptorSet writeDescSet2{
        .dstSet = combinerDescriptors.resource->sets[0],
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Conform to the type at binding.
        .pImageInfo = &imageinfos[1]
    };
    combinerDescriptors.resource->WriteSet(vkmod.vk, writeDescSet2);
}

void IndexVK::InitCombinerPipeline(VkModule& vkmod, ResourceOwners& resources){
    PipelineBuilder pb;

    // descriptors needed only.
    pb.defDescriptorLayouts.push_back(combinerDescriptorLayout.resource);

    // triangle list drawing setup
    pb.defInputAssemblyCI = {
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    // shaders
    pb.defShaderStageCIs.push_back({
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = combinerShader.resource, .pName = "main"
    });
    pb.defShaderStageCIs.push_back({
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = combinerShader.resource, .pName = "main"
    });

    // dynamic viewport and scisor for resizing
    pb.defViewportCI = {
        .viewportCount = 1,
        .scissorCount = 1
    };
    pb.defDynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    pb.defDynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

    // color attachments
    pb.defAttachmentFormats.push_back(vkmod.ssc.swapchainci.imageFormat);
    pb.defAttachmentBlends.push_back(GraphicsPipeline::basicColorBlendAttachmentci());

    // depth stencil
    pb.defDepthStencilCI = {
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };
    pb.defDepthFormat = vkmod.sscdepth.imageci.format;

    // unused but needed line width and multisampling setup
    pb.defRasterizationCI = GraphicsPipeline::basicRasterizationci();
    pb.defMultiSampleCI = GraphicsPipeline::basicMultisampleci();

    combinerPipeline = resources.pipelines.Add("Index_Combiner_Pipeline", pb.Build(vkmod.vk));
}

void IndexVK::Resize(VkModule& vkmod){
    // Redo Depth and Color textures.
    depth.resource->Resize(vkmod.vk, vkmod.windowSize);
    coltex.resource->Resize(vkmod.vk, vkmod.windowSize);
    idtex.resource->Resize(vkmod.vk, vkmod.windowSize);

    uint32_t idtexsize = static_cast<uint32_t>(vkmod.windowSize.x * vkmod.windowSize.y) * sizeof(uint32_t);
    for (int i = 0; i < VkModule::maxFramesInFlight; i++) {
        idbufs[i].resource->Resize(vkmod.vk, idtexsize);
    }

    // Redo just the Descriptor Set, so that it points to the correct resources.
    std::vector<VkDescriptorImageInfo> imageinfos = {
        {
            .sampler = coltex.resource->sampler,
            .imageView = coltex.resource->view, // This now points to new view.
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
        },
        {
            .sampler = idtex.resource->sampler,
            .imageView = idtex.resource->view, // This now points to new view.
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
        }
    };
    VkWriteDescriptorSet writeDescSet1{
        .dstSet = combinerDescriptors.resource->sets[0], // Write to set 0, again.
        .dstBinding = 0, // The 0 bound descriptor in the set.
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Conform to the type at binding.
        .pImageInfo = &imageinfos[0]
    };
    combinerDescriptors.resource->WriteSet(vkmod.vk, writeDescSet1);
    VkWriteDescriptorSet writeDescSet2{
        .dstSet = combinerDescriptors.resource->sets[0],
        .dstBinding = 1, // The 1 bound descriptor in the set.
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Conform to the type at binding.
        .pImageInfo = &imageinfos[1]
    };
    combinerDescriptors.resource->WriteSet(vkmod.vk, writeDescSet2);
}

uint32_t IndexVK::readId(glm::ivec2 windowSize, glm::ivec2 mouseCoord, uint32_t frameIndex){
    int idx = mouseCoord.x + mouseCoord.y * windowSize.x;
    return static_cast<uint32_t*>(idbufs[frameIndex].resource->buf.allocationInfo.pMappedData)[idx];
}

void IndexVK::BeginRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& nextsscimage, SurfaceSwapChain& ssc, DepthTexture& sscdepth, uint32_t& frameIndex){
    // transition image layouts
    // src: previous stage, and its usage (access) of the image
    // dst: next stage, and its usage (access) of the image
    // old: barrier transforms from this layout
    // new: to this layout
    // src -> barrier(old->new) -> dst

    cmds.ibc.Add({
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // We will download from this texture.
        .image = idtex.resource->image,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
    });
    cmds.ibc.RecordAndClearCommands(cmds.cb);

    VkBufferImageCopy copyregions{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = { .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel=0, .baseArrayLayer=0, .layerCount=1},
        .imageOffset = {.x=0, .y=0, .z=0},
        .imageExtent = {.width=static_cast<uint32_t>(windowSize.x), .height=static_cast<uint32_t>(windowSize.y), .depth=1}
    };
    vkCmdCopyImageToBuffer(cmds.cb, idtex.resource->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, idbufs[frameIndex].resource->buf.handle, 1, &copyregions);

    VkBufferMemoryBarrier2 bufferbarrierci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
        .dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
        .buffer = idbufs[frameIndex].resource->buf.handle,
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };
    VkDependencyInfo barrierci{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &bufferbarrierci
    };
    vkCmdPipelineBarrier2(cmds.cb, &barrierci); // Memory barrier on buffer, just so copy is handled robustly


    cmds.ibc.Add({
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .image = idtex.resource->image,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
    });
    cmds.ibc.Add({
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .image = ssc.images[nextsscimage], // set color attachment (imageIndex is next image)
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
    });
    cmds.ibc.Add({
        .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .image = sscdepth.image, // set depth attachment
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 }
    });
    cmds.ibc.RecordAndClearCommands(cmds.cb);

    // setup dynamic rendering info
    cmds.drc.Add({
        .imageView = ssc.views[nextsscimage],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue{.color{ 0.0f, 0.0f, 0.2f, 1.0f }}
    });
    cmds.drc.depth = {
        .imageView = sscdepth.view,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = {.depthStencil = {1.0f,  0}}
    };
    cmds.drc.BeginDynamicRendering(cmds.cb, windowSize);

}

void IndexVK::RecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& nextsscimage, SurfaceSwapChain& ssc, DepthTexture& sscdepth){
    VkViewport vp{
        .width = static_cast<float>(windowSize.x),
        .height = static_cast<float>(windowSize.y),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(cmds.cb, 0, 1, &vp);

    VkRect2D scissor{ .extent{ .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) } };
    vkCmdSetScissor(cmds.cb, 0, 1, &scissor);

    // combiner pipeline
    vkCmdBindPipeline(cmds.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, combinerPipeline.resource->handle);

    // texture descriptors
    vkCmdBindDescriptorSets(cmds.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, combinerPipeline.resource->layout, 0, 1, &combinerDescriptors.resource->sets[0], 0, nullptr);

    // draw
    vkCmdDraw(cmds.cb, 3, 1, 0, 0);
}

void IndexVK::EndRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& nextsscimage, SurfaceSwapChain& ssc, DepthTexture& sscdepth){

    cmds.drc.EndDynamicRendering(cmds.cb);

    // switch layout to presentation
    cmds.ibc.Add(
        {
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = ssc.images[nextsscimage],
            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
        }
    );
    cmds.ibc.RecordAndClearCommands(cmds.cb);
}




void DuckVk::Init(VkModule& vkmod, IndexVK& indexVk, ResourceOwners &resources){
    model = resources.models.Add("Duck_obj", ObjModel::Build(vkmod.vk, "assets/duck.obj"));

    auto duck_texture = SamplerTexture();
    duck_texture.InitFromFile(vkmod.vk, vkmod.commandPool, vkmod.queue, "assets/duck.ktx");
    textures[0] = resources.samplertextures.Add("Duck_Texture", std::move(duck_texture));

    for (auto i = 0; i < VkModule::maxFramesInFlight; i++) {
        std::string name = "Duck_Uniform_Frame" + std::to_string(i);
        uniforms[i] = resources.shaderbuffers.Add(name, ShaderBuffer::Build<DuckVk::Uniforms>(vkmod.vk));
    }

    InitDescriptors(vkmod, resources);

    VkShaderModule duck_shader = {VK_NULL_HANDLE};
    vkmod.slang.LoadAndCompile(vkmod.vk, std::string("shaders/ducks.slang"), duck_shader);
    shader = resources.shaders.Add("Duck_Shader", std::move(duck_shader));

    InitPipeline(vkmod, indexVk, resources);
}

void DuckVk::InitDescriptors(VkModule &vkmod, ResourceOwners& resources){
    // descriptor layout (column definitions)
    DescriptorLayoutBuilder layoutbuilder{};
    layoutbuilder.Add(
        {
            .binding = 0, // location in shader
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // type
            .descriptorCount = static_cast<uint32_t>(textures.size()), // number of combined images
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        },
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
    );
    descriptorlayout = resources.descriptorlayouts.Add("Duck_Descriptor_Layout", layoutbuilder.Build(vkmod.vk));

    // descriptor pool (table from columns)
    DescriptorPoolBuilder poolbuilder{};
    poolbuilder.Add(
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(textures.size()) // hold at MAX this many TYPES (imagesamplers)
        }
    );
    descriptorpool = resources.descriptorpools.Add("Duck_Descriptors", Descriptors{.pool= poolbuilder.Build(vkmod.vk, 1)});

    // descriptor set alloc (add a row to table)
    DescriptorSetAllocator setallocator{};
    setallocator.Add(
        descriptorlayout.resource,
        static_cast<uint32_t>(textures.size())
    );
    setallocator.Allocate(vkmod.vk, *descriptorpool.resource);

    // descriptor upload (field of row assignment)
    std::vector<VkDescriptorImageInfo> imageinfos;
    for (auto& texture : textures) {
        imageinfos.push_back(VkDescriptorImageInfo{
            .sampler = texture.resource->sampler,
            .imageView = texture.resource->view,
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
        });
    }

    VkWriteDescriptorSet writeDescSet{
        .dstSet = descriptorpool.resource->sets[0],
        .dstBinding = 0,
        .descriptorCount = static_cast<uint32_t>(imageinfos.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // conform to the type at binding
        .pImageInfo = imageinfos.data()
    };
    descriptorpool.resource->WriteSet(vkmod.vk, writeDescSet);
}

void DuckVk::InitPipeline(VkModule &vkmod, IndexVK& indexVk, ResourceOwners &resources){
    PipelineBuilder pb;

    // pushconstants and descriptors in the pipeline.
    pb.defPushConstants.push_back({
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .size = sizeof(VkDeviceAddress)
        });
    pb.defDescriptorLayouts.push_back(descriptorlayout.resource);

    // whole buffer consists of multiple vertex structures
    pb.defVertexBindings.push_back({
            .binding = 0,
            .stride = sizeof(ObjModel::Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    });
    pb.defVertexAttributes.push_back(
        { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT }
    );
    pb.defVertexAttributes.push_back(
        { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(ObjModel::Vertex, normal) }
    );
    pb.defVertexAttributes.push_back(
        { .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ObjModel::Vertex, uv) }
    );
    pb.defInputAssemblyCI = {
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    // shader stages
    pb.defShaderStageCIs.push_back({
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shader.resource, .pName = "main"
    });
    pb.defShaderStageCIs.push_back({
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shader.resource, .pName = "main"
    });

    //viewport and scisor ci
    pb.defViewportCI = {
        .viewportCount = 1,
        .scissorCount = 1
    };
    pb.defDynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    pb.defDynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

    // color attachments (formats coming from vkmod)
    pb.defAttachmentFormats.push_back(indexVk.coltex.resource->imageci.format);
    pb.defAttachmentFormats.push_back(indexVk.idtex.resource->imageci.format);
    pb.defAttachmentBlends.push_back(GraphicsPipeline::basicColorBlendAttachmentci());
    pb.defAttachmentBlends.push_back({
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
    });

    // depth stencil
    pb.defDepthStencilCI = {
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };
    pb.defDepthFormat = indexVk.depth.resource->imageci.format;

    // line width and multisampling setup
    pb.defRasterizationCI = GraphicsPipeline::basicRasterizationci();
    pb.defMultiSampleCI = GraphicsPipeline::basicMultisampleci();

    pipeline = resources.pipelines.Add("Duck_Pipeline", pb.Build(vkmod.vk));
}

void DuckVk::BeginRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, SamplerTexture& coltex, DepthTexture& coldepth, SamplerTexture& idtex){
    cmds.ibc.Add(
        {
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = coltex.image,
            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
        }
    );
    cmds.ibc.Add(
        {
            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = coldepth.image, // set depth attachment
            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 }
        }
    );
    cmds.ibc.Add(
        {
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = idtex.image,
            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
        }
    );
    cmds.ibc.RecordAndClearCommands(cmds.cb);

    cmds.drc.Add(
        {
            .imageView = coltex.view,
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue{.color{ 0.05f, 0.06f, 0.075f, 1.0f }}
        }
    );
    cmds.drc.Add(
        {
            .imageView = idtex.view,
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue{.color{.uint32 = {0, 0, 0, 0}}}
        }
    );
    cmds.drc.depth = {
        .imageView = coldepth.view,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = {.depthStencil = {1.0f,  0}}
    };
    cmds.drc.BeginDynamicRendering(cmds.cb, windowSize);
}

void DuckVk::RecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& frameIndex){
    // change viewport
    VkViewport vp{
        .width = static_cast<float>(windowSize.x),
        .height = static_cast<float>(windowSize.y),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(cmds.cb, 0, 1, &vp);

    // set culling
    VkRect2D scissor{ .extent{ .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) } };
    vkCmdSetScissor(cmds.cb, 0, 1, &scissor);

    // shader
    vkCmdBindPipeline(cmds.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.resource->handle);

    // texture descriptors
    vkCmdBindDescriptorSets(cmds.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.resource->layout, 0, 1, &descriptorpool.resource->sets[0], 0, nullptr);

    // add uniforms
    vkCmdPushConstants(cmds.cb, pipeline.resource->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VkDeviceAddress), &uniforms[frameIndex].resource->deviceAddress);

    // vertex buffer
    VkDeviceSize vOffset{ 0 };
    vkCmdBindVertexBuffers(cmds.cb, 0, 1, &model.resource->buf.handle, &vOffset);

    // indexes are in the same buffer, after vertexes
    VkDeviceSize vertexSize = {model.resource->vertexcount * sizeof(ObjModel::Vertex)};
    vkCmdBindIndexBuffer(cmds.cb, model.resource->buf.handle, vertexSize, VK_INDEX_TYPE_UINT16);

    // draw
    vkCmdDrawIndexed(cmds.cb, model.resource->indexcount, 3, 0, 0, 0);
}

void DuckVk::EndRecordCommands(CommandRecorders& cmds, glm::ivec2& windowSize, SamplerTexture& coltex, SamplerTexture& idtex){

    cmds.drc.EndDynamicRendering(cmds.cb);

    // transition coltex
    cmds.ibc.Add(
        {
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image = coltex.image,
            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
        }
    );
    cmds.ibc.RecordAndClearCommands(cmds.cb);
}



void GuiVk::update(ImGuiModule& guimod, VkModule& vkmod, VkFence& fence, IndexVK& indexVk, const uint32_t& frameIndex, const uint32_t& prevIndex, glm::vec2& mousePos, uint32_t& id){
    if (ImGui::Begin("Demo", &shown)){
        ImGui::Text("A demo window.");

        ImGui::Text("Resolution: %d x %d", vkmod.windowSize.x, vkmod.windowSize.y);
        ImGui::Text("MousePos: (%.2f;%.2f)", mousePos.x, mousePos.y);

        ImGui::Checkbox("Read under mouse.", &readUnderMouse);
        if (readUnderMouse) {
            chk(vkWaitForFences(vkmod.vk.ldevice.handle, 1, &fence, true, UINT64_MAX));
            id = indexVk.readId(vkmod.windowSize, mousePos, frameIndex);
            ImGui::SameLine();
            ImGui::Text("Mouse id is: %d", id);
        }

        ImGui::Checkbox("Plot indexes every frame.", &plotEveryFrame);
        if (plotEveryFrame) {
            chk(vkWaitForFences(vkmod.vk.ldevice.handle, 1, &fence, true, UINT64_MAX));
            fillIds(vkmod, *indexVk.idbufs[prevIndex].resource);
        }else{
            ImGui::SameLine();
            if (ImGui::Button("Wait for indexes...")) {
                chk(vkWaitForFences(vkmod.vk.ldevice.handle, 1, &fence, true, UINT64_MAX));
                fillIds(vkmod, *indexVk.idbufs[prevIndex].resource);
            }
        }

        if (!ids.empty()) {
            ImGui::Text("iddim: %d x %d", iddim.x, iddim.y);
            if (ImPlot::BeginPlot("ids plot", ImVec2(static_cast<float>(iddim.x)*5,static_cast<float>(iddim.y)*5), ImPlotFlags_NoInputs)) {
                ImPlot::PlotHeatmap(
                    "ids",
                    ids.data(), iddim.y, iddim.x,
                    0,
                    0,
                    nullptr
                );
                ImPlot::EndPlot();
            }
        }

    }
    ImGui::End();
}

void GuiVk::fillIds(VkModule& vkmod, StagingBuffer& idbuf){
    //int itemnum = vkmod.windowSize.x*vkmod.windowSize.y;
    //ids.resize(itemnum);
    //memcpy(ids.data(), static_cast<uint32_t*>(idbuf.buf.allocationInfo.pMappedData), itemnum*sizeof(uint32_t));

    ids.clear();
    iddim = vkmod.windowSize / sampleStep;

    for (int y=0; y<iddim.y; y+=1) {
        for (int x=0; x<iddim.x; x+=1) {
            int sampledX = x*sampleStep;
            int sampledY = y*sampleStep;
            int sampledIdx = (sampledX+sampledY*vkmod.windowSize.x);
            uint32_t num = (static_cast<uint32_t*>(idbuf.buf.allocationInfo.pMappedData))[sampledIdx];

            ids.push_back(num);
        }
    }
}



void App::Init(Harness<App>::Modules& m){
    uint32_t deviceID = 0;

    resources.Init(&m.vkmod.vk);
    resources.name = "App";

    indexVk.Init(m.vkmod, resources);
    duckVk.Init(m.vkmod,  indexVk, resources);
}

void App::cursorPosChanged(GLFWwindow *window, double xpos, double ypos){
     auto app = static_cast<App*>(glfwGetWindowUserPointer(window));
     app->mousePos = glm::vec2(xpos, ypos);
}

void App::Destroy(Harness<App>::Modules& m){
    // App default destructor auto destroys everything currently.
}

void App::printApiData(Harness<App>::Modules& m){
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );
    std::cout << extensionCount << " extensions supported\n";

    uint32_t version = 0;
    vkEnumerateInstanceVersion(&version);
    std::cout
                << VK_API_VERSION_MAJOR(version) << " - "
                << VK_API_VERSION_MINOR(version) << " - "
                << VK_API_VERSION_PATCH(version) <<
    std::endl;

    std::cout << ImGui::GetVersion() << std::endl;
}

void App::updateDeltaTime(Harness<App>::Modules& m){
    oldTime = time;
    time = glfwGetTime();
    deltaTime = time - oldTime;
}

void App::updateShaderData(Harness<App>::Modules& m){
    shaderData.projection = glm::perspective(glm::radians(45.0f), (float)m.vkmod.windowSize.x / (float)m.vkmod.windowSize.y, 0.1f, 32.0f);
    shaderData.view = glm::lookAt(camPos, glm::vec3(0.0), glm::vec3(0.0,1.0,0.0));
    for (auto i = 0; i < 3; i++) {
        auto instancePos = glm::vec3((float)(i - 1) * 5.0f, 0.0f, 0.0f);
        shaderData.model[i] =
            glm::translate(glm::mat4(1.0f), instancePos) * glm::mat4_cast(glm::quat(objectRotations[i])) *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.0) * 0.5f);
    }
    memcpy(duckVk.uniforms[frameIndex].resource->buf.allocationInfo.pMappedData, &shaderData, sizeof(DuckVk::Uniforms));
}

void App::updateFrameIndex(){
    frameIndex = (frameIndex + 1) % VkModule::maxFramesInFlight;
}

uint32_t App::previousFrameIdx(){
    return (frameIndex-1)==-1 ? (VkModule::maxFramesInFlight-1) : (frameIndex-1);
}

void App::pollEvents(Harness<App>::Modules& m){
    glfwPollEvents();
}

void App::updateAppLogic(Harness<App>::Modules &m){
    float y = -5.0;
    camPos = glm::vec3(cos(time),0.0,sin(time)) * 15.0f + glm::vec3(0.0,y,0.0);
}

void App::Play(Harness<App>::Modules& m){
    printApiData(m);
    m.glfwmod.printWindowDimensions();

    while (!glfwWindowShouldClose(m.glfwmod.window)){
        updateFrameIndex();
        m.vkmod.updateFence(frameIndex); // Wait for this frame fence and reset it
        m.vkmod.updateImage(frameIndex,imageIndex,updateSwapchain); // Acquire next ssc image.
        while (updateSwapchain) {
            chk(vkDeviceWaitIdle(m.vkmod.vk.ldevice.handle));
            m.vkmod.recreateSwapchains(m.glfwmod);
            m.guimod.recreateImGui(m.vkmod);
            indexVk.Resize(m.vkmod);
            m.vkmod.updateImage(frameIndex,imageIndex,updateSwapchain);
        }

        updateDeltaTime(m);

        pollEvents(m);

        m.guimod.beginFrame();
        guiVk.update(m.guimod, m.vkmod, m.vkmod.fences[previousFrameIdx()], indexVk, frameIndex, previousFrameIdx(), mousePos, shaderData.id);
        m.guimod.endFrame();

        updateAppLogic(m);

        updateShaderData(m);

        cmds.cb = m.vkmod.commandBuffers[frameIndex];
        m.vkmod.BeginCBRecordingToSSC(frameIndex);

        duckVk.BeginRecordCommands(cmds, m.vkmod.windowSize, *indexVk.coltex.resource, *indexVk.depth.resource, *indexVk.idtex.resource);
        duckVk.RecordCommands(cmds, m.vkmod.windowSize, frameIndex);
        duckVk.EndRecordCommands(cmds, m.vkmod.windowSize, *indexVk.coltex.resource, *indexVk.idtex.resource);

        indexVk.BeginRecordCommands(cmds, m.vkmod.windowSize, imageIndex, m.vkmod.ssc, m.vkmod.sscdepth, frameIndex);
        indexVk.RecordCommands(cmds, m.vkmod.windowSize, imageIndex, m.vkmod.ssc, m.vkmod.sscdepth);
        m.guimod.RecordRenderCommands(frameIndex, m.vkmod);
        indexVk.EndRecordCommands(cmds, m.vkmod.windowSize, imageIndex, m.vkmod.ssc, m.vkmod.sscdepth);

        m.vkmod.EndCBRecordingToSSC(frameIndex);
        m.vkmod.updateSubmitCommandBuffer(frameIndex,imageIndex);
        m.vkmod.updatePresentImage(imageIndex,updateSwapchain);
    }
}
