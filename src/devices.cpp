#include "kanoc/devices.hpp"

void LogicalDevice::Destroy(){
    vkDestroyDevice(handle, nullptr);
}

void VmaFields::Destroy(){
    vmaDestroyAllocator(allocator);
}

void DestroyVKHandle(VkInstance instance){
    vkDestroyInstance(instance, nullptr);
}

void FullDevice::Destoy(){
    vma.Destroy();
    ldevice.Destroy();
    DestroyVKHandle(instance);
}
