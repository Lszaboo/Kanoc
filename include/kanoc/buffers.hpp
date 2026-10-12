#pragma once

#include <cstdint>
#include <glm/fwd.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <tiny_obj_loader.h>

#include "kanoc/devices.hpp"

struct Buffer{
    VkBuffer handle = {VK_NULL_HANDLE};
    VmaAllocation allocation = {VK_NULL_HANDLE};
    VmaAllocationInfo allocationInfo = {};

    void Destroy(FullDevice& vk);
};

/*
 * Builds Buffer structs.
 * Set the fields first, then call build.
 */
class BufferBuilder{
    public:
        VkBufferCreateInfo bufferCI;
        VmaAllocationCreateInfo bufferAllocCI;

        Buffer Build(FullDevice& vk);
};

class ObjModel{
    public:
        Buffer buf;
        uint32_t indexcount;
        uint32_t vertexcount;

        static ObjModel Build(FullDevice& vk, const std::string& path);

        void Destroy(FullDevice& vk);

        struct Vertex{
            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec2 uv;
        };

    protected:
        static void parseVerticesAndIndices(tinyobj::attrib_t& attrib, tinyobj::shape_t& shape, std::vector<Vertex>& vertices, std::vector<glm::uint16_t>& indices);
};

/*
 * Buffer mostly used for:
 * - uploading data to GPU through it.
 * - reading data from the GPU through it.
 * Use buf.allocationInfo.pMappedData to acces mapped pointer.
 * Use (char*) for data read and write.
 */
class StagingBuffer{
    public:
        Buffer buf;
        uint32_t size;

        static StagingBuffer Build(FullDevice& vk, uint32_t size);
        static StagingBuffer Build(FullDevice& vk, uint32_t size, char* data);
        void Destroy(FullDevice& vk);
};

class VertexBuffer {
    public:
        VkBuffer handle = {VK_NULL_HANDLE};
        VmaAllocation allocation = {VK_NULL_HANDLE};

        uint32_t indexCount;
        uint32_t vertexCount;

        void Destroy(FullDevice& vk);
};
