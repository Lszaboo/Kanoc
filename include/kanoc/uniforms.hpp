#pragma once

#include "kanoc/debug.hpp"
#include "kanoc/buffers.hpp"
#include "kanoc/devices.hpp"
#include "textures.hpp"
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

struct ShaderDataBuffer {
    Buffer buf;
	VkDeviceAddress deviceAddress{};

	template<typename UniformT>
	static ShaderDataBuffer Build(FullDevice& vk){
	    BufferBuilder bb;
		bb.bufferCI = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(UniformT),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        };
		bb.bufferAllocCI = {
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
		return ShaderDataBuffer{
		    .buf = buf,
			.deviceAddress = vkGetBufferDeviceAddress(vk.ldevice.handle, &uBufferBdaInfo)
		};
	}

	void Destroy(FullDevice& vk){
	    buf.Destroy(vk);
	}
};
