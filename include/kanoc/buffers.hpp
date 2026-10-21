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
        VkBufferCreateInfo defBufferCI;
        VmaAllocationCreateInfo defBufferAllocCI;

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
 * StagingBuffer mostly used for:
 * - uploading data to GPU through it.
 * - downloading data from the GPU through it.
 * Use buf.allocationInfo.pMappedData to acces mapped pointer.
 * Use (char*) for data read and write.
 */
class StagingBuffer{
    public:
        BufferBuilder bb;
        Buffer buf;
        uint32_t size;

        static StagingBuffer BuildForUpload(FullDevice& vk, uint32_t size);
        static StagingBuffer BuildForUpload(FullDevice& vk, uint32_t size, char* data);
        static StagingBuffer BuildForDownload(FullDevice& vk, uint32_t size);
        void Destroy(FullDevice& vk);

        void Resize(FullDevice& vk, uint32_t size);
};

struct ShaderBuffer {
    Buffer buf;
	VkDeviceAddress deviceAddress{};

	template<typename UniformT>
	static ShaderBuffer Build(FullDevice& vk){
	    BufferBuilder bb;
		bb.defBufferCI = {
            .size = sizeof(UniformT),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        };
		bb.defBufferAllocCI = {
            .flags =    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
		};
		Buffer buf = bb.Build(vk);

		VkBufferDeviceAddressInfo uBufferBdaInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buf.handle
        };
		return ShaderBuffer{
		    .buf = buf,
			.deviceAddress = vkGetBufferDeviceAddress(vk.ldevice.handle, &uBufferBdaInfo)
		};
	}

	void Destroy(FullDevice& vk){
	    buf.Destroy(vk);
	}
};
