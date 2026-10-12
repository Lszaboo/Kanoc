#pragma once

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"
#include "kanoc/devices.hpp"
#include <vulkan/vulkan_core.h>

void DestroyVkHandle(FullDevice& vk, VkShaderModule& shader);

class SlangCompiler{
    public:
        Slang::ComPtr<slang::IGlobalSession> globalsession;
        Slang::ComPtr<slang::ISession> session;

        void Init();
        void LoadAndCompile(FullDevice& vk, std::string path, VkShaderModule& shader);

};
