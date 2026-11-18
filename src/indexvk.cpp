#include "kanoc/indexvk.hpp"

#include "kanoc/chk.hpp"

void IndexVK::InitIndexes(Modules *m, ResourceOwners &resources){
    auto index_texture = SamplerTexture();
    index_texture.InitIDReadWrite(m->vkmod.vk, m->vkmod.windowDims);
    idtex = resources.samplertextures.Add("Index_Index_Texture", std::move(index_texture));

    uint32_t idtexsize = static_cast<uint32_t>(m->vkmod.windowDims.x * m->vkmod.windowDims.y) * sizeof(uint32_t);
    for (int i = 0; i < VkModule::maxFramesInFlight; i++) {
        std::string name = "Index_Index_Buffer" + std::to_string(i);
        idbufs[i] = resources.stagingbuffers.Add(name, StagingBuffer::BuildForDownload(m->vkmod.vk, idtexsize));
        name = "Index_Index_Fence" + std::to_string(i);
        idfences[i] = resources.fences.Add(name, BuildFence(m->vkmod.vk, {}));
    }
}

void IndexVK::InitShader(Modules* m, ResourceOwners& resources){
    VkShaderModule combiner_shader = {VK_NULL_HANDLE};
    // FIXME: combiner.slang should be included with Kanoc.
    m->vkmod.slang.LoadAndCompile(m->vkmod.vk, m->workingPath + "/shaders/combiner.slang", combiner_shader);
    combinerShader = resources.shaders.Add("Index_Combiner_Shader", std::move(combiner_shader));
}

void IndexVK::InitDescriptors(Modules* m, ResourceOwners& resources){
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
    combinerDescriptorLayout = resources.descriptorlayouts.Add("Index_Combiner_Descriptor_Layout", layoutBuilder.Build(m->vkmod.vk));

    DescriptorPoolBuilder poolbuilder{};
    poolbuilder.Add({
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1
    });
    combinerDescriptors = resources.descriptorpools.Add("Index_Combiner_Descriptors", {.pool = poolbuilder.Build(m->vkmod.vk, 1)});

    DescriptorSetAllocator setAllocator{};
    setAllocator.Add(
        combinerDescriptorLayout.resource,
        1 // Since it is not a Variable sized layout, Vulkan doesn't really care.
    );
    setAllocator.Allocate(m->vkmod.vk, *combinerDescriptors.resource);

    // Upload descriptor set data.
    VkDescriptorImageInfo imageinfo = {
        .sampler = idtex.resource->sampler,
        .imageView = idtex.resource->view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkWriteDescriptorSet writeDescSet{
        .dstSet = combinerDescriptors.resource->sets[0],
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Conform to the type at binding.
        .pImageInfo = &imageinfo
    };
    combinerDescriptors.resource->WriteSet(m->vkmod.vk, writeDescSet);
}

void IndexVK::InitCombinerPipeline(Modules* m, ResourceOwners& resources, std::vector<VkDescriptorSetLayout>& descriptorLayouts){
    PipelineBuilder pb;

    // descriptors needed only.
    pb.defDescriptorLayouts.push_back(combinerDescriptorLayout.resource);

    // Extend with user descriptors.
    for (auto& descriptorLayout : descriptorLayouts) {
        pb.defDescriptorLayouts.push_back(descriptorLayout);
    }

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
    pb.defAttachmentFormats.push_back(m->vkmod.ssc.swapchainci.imageFormat);
    pb.defAttachmentBlends.push_back(GraphicsPipeline::basicColorBlendAttachmentci());

    // depth stencil
    pb.defDepthStencilCI = {
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };
    pb.defDepthFormat = m->vkmod.ssc.depth.imageci.format;

    // unused but needed line width and multisampling setup
    pb.defRasterizationCI = GraphicsPipeline::basicRasterizationci();
    pb.defMultiSampleCI = GraphicsPipeline::basicMultisampleci();

    combinerPipeline = resources.pipelines.Add("Index_Combiner_Pipeline", pb.Build(m->vkmod.vk));
}

void IndexVK::Resize(VkModule& vkmod){
    // Redo id texture.
    idtex.resource->Resize(vkmod.vk, vkmod.windowDims);

    uint32_t idtexsize = static_cast<uint32_t>(vkmod.windowDims.x * vkmod.windowDims.y) * sizeof(uint32_t);
    for (int i = 0; i < VkModule::maxFramesInFlight; i++) {
        idbufs[i].resource->Resize(vkmod.vk, idtexsize);
    }

    // Redo just the Descriptor Set, so that it points to the correct resources.
    VkDescriptorImageInfo imageinfo = {
            .sampler = idtex.resource->sampler,
            .imageView = idtex.resource->view, // This now points to new view.
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
    };
    VkWriteDescriptorSet writeDescSet{
        .dstSet = combinerDescriptors.resource->sets[0], // Write to set 0, again.
        .dstBinding = 0, // The 0 bound descriptor in the set.
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Conform to the type at binding.
        .pImageInfo = &imageinfo
    };
    combinerDescriptors.resource->WriteSet(vkmod.vk, writeDescSet);
}

void IndexVK::waitIdTexComplete(Modules *m){
    VkFence* prevFence = &m->vkmod.frameFinishedFences[VkModule::previousFrameIdx(m->vkmod.frameIdx)];
    chk(vkWaitForFences(m->vkmod.vk.ldevice.handle, 1, prevFence, true, UINT64_MAX));
}

uint32_t IndexVK::readIdTex(glm::ivec2 windowSize, glm::ivec2 mouseCoord, uint32_t frameIndex){
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
        .image = ssc.depth.image, // set depth attachment
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
        .imageView = ssc.depth.view,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = {.depthStencil = {1.0f,  0}}
    };
    cmds.drc.BeginDynamicRendering(cmds.cb, windowSize);

}

void IndexVK::RecordBindCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& nextsscimage, SurfaceSwapChain& ssc, DepthTexture& sscdepth){
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
}

void IndexVK::RecordDrawCommands(CommandRecorders& cmds, glm::ivec2& windowSize, uint32_t& nextsscimage, SurfaceSwapChain& ssc, DepthTexture& sscdepth){
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
