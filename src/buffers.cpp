#include "kanoc/buffers.hpp"

#include <cstdint>
#include <cstring>
#include <vulkan/vulkan_core.h>

#include "kanoc/chk.hpp"
#include "kanoc/devices.hpp"

void Buffer::Destroy(FullDevice &vk){
    vmaDestroyBuffer(vk.vma.allocator, handle, allocation);
}

Buffer BufferBuilder::Build(FullDevice& vk){
    Buffer buf;

    defBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    chk(vmaCreateBuffer(vk.vma.allocator, &defBufferCI, &defBufferAllocCI, &buf.handle, &buf.allocation, &buf.allocationInfo));

    return buf;
}

void ObjModel::parseVerticesAndIndices(tinyobj::attrib_t& attrib, tinyobj::shape_t& shape, std::vector<Vertex>& vertices, std::vector<glm::uint16_t>& indices){

    auto& coords = attrib.vertices;
    auto& normals = attrib.normals;
    auto& texcoords = attrib.texcoords;

    for (tinyobj::index_t index : shape.mesh.indices) {
        int vert_idx = index.vertex_index*3;
        int norm_idx = index.normal_index*3;
        int uv_idx = index.texcoord_index*2;

        Vertex vert{
            .pos =      {coords[vert_idx+0],    -coords[vert_idx+1],    coords[vert_idx+2]},
            .normal =   {normals[norm_idx+0],   -normals[norm_idx+1],   normals[norm_idx+2]},
            .uv =       {texcoords[uv_idx+0],1.0-texcoords[uv_idx+1]}
        };

        vertices.push_back(vert);
        indices.push_back(indices.size());
    }
}

ObjModel ObjModel::Build(FullDevice& vk, const std::string& path){
    ObjModel objmodel;

    tinyobj::attrib_t attrib; // Vertex data
    std::vector<tinyobj::shape_t> shapes; // Indices
    std::vector<tinyobj::material_t> materials; // Materials
    chk_obj(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, path.c_str()));

    std::vector<Vertex> vertices;
    std::vector<glm::uint16_t> indices;
    ObjModel::parseVerticesAndIndices(attrib, shapes[0], vertices, indices);

    VkDeviceSize vertexSize = { sizeof(Vertex) * vertices.size() };
    VkDeviceSize indexSize = { sizeof(uint16_t) * indices.size() };

    BufferBuilder bb;
    bb.defBufferCI = {
        .size = vertexSize + indexSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };
    bb.defBufferAllocCI = {
        // try and give me DEVICE_LOCAL (VRAM) and HOST_ACCES-able (CPU) memory.
        // BAR and Shared Memory works here.
        // it can fail, by giving me DEVICE_LOCAL not HOST_ACCES-able memory
        // if it's the case, Staging Buffer, then GPU Buffer needed.
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    objmodel.buf = bb.Build(vk);

    memcpy(objmodel.buf.allocationInfo.pMappedData,                        vertices.data(), vertexSize);
    memcpy(((char*)objmodel.buf.allocationInfo.pMappedData) + vertexSize,  indices.data(), indexSize);

    objmodel.vertexcount = vertices.size();
    objmodel.indexcount = indices.size();

    return objmodel;
}

void ObjModel::Destroy(FullDevice &vk){
    buf.Destroy(vk);
}

StagingBuffer StagingBuffer::BuildForUpload(FullDevice& vk, uint32_t size){
    BufferBuilder bb;

    bb.defBufferCI = {
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT // Can upload to GPU from it.
    };
    bb.defBufferAllocCI = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    return  {
        .buf = bb.Build(vk),
        .size = size
    };
}

StagingBuffer StagingBuffer::BuildForUpload(FullDevice& vk, uint32_t size, char* data){
    StagingBuffer sbuf = BuildForUpload(vk, size);
    memcpy(sbuf.buf.allocationInfo.pMappedData, data, size);
    return  sbuf;
}

StagingBuffer StagingBuffer::BuildForDownload(FullDevice &vk, uint32_t size){
    BufferBuilder bb;

    bb.defBufferCI = {
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT // Can download from GPU to it.
    };
    bb.defBufferAllocCI = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    return {
        .buf = bb.Build(vk),
        .size = size
    };
}

void StagingBuffer::Destroy(FullDevice& vk){
    buf.Destroy(vk);
}
