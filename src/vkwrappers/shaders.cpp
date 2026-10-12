#include "vkwrappers/shaders.hpp"
#include "chk.hpp"

#include <array>
#include <string>
#include <fstream>
#include <iostream>

void DestroyVkHandle(FullDevice& vk, VkShaderModule& shader){
    vkDestroyShaderModule(vk.ldevice.handle, shader, nullptr);
}

void SlangCompiler::Init(){
    // init global Slang session
    slang::createGlobalSession(this->globalsession.writeRef());

    // init Slang 2 Spir-V session
    auto slangTargets{ std::to_array<slang::TargetDesc>({ {
        .format = SLANG_SPIRV,
        .profile = this->globalsession->findProfile("spirv_1_4")
    } })};
    auto slangOptions{ std::to_array<slang::CompilerOptionEntry>({ {
        slang::CompilerOptionName::EmitSpirvDirectly,
        {slang::CompilerOptionValueKind::Int, 1}
    } })};
    slang::SessionDesc slangSessionDesc{
        .targets = slangTargets.data(),
        .targetCount = SlangInt(slangTargets.size()),
        .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
        .compilerOptionEntries = slangOptions.data(),
        .compilerOptionEntryCount = uint32_t(slangOptions.size())
    };
    this->globalsession->createSession(slangSessionDesc, this->session.writeRef());
}

void SlangCompiler::LoadAndCompile(FullDevice& vk, std::string path, VkShaderModule& shader){
    // load shader from disk
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        std::cerr << "File open failed!" << std::endl;
    }
    std::string source{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>() // read until begin reaches end-of-iterator state (end)
    };


    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    Slang::ComPtr<slang::IModule> slangModule(this->session->loadModuleFromSourceString(
        std::string(path + "_shader").c_str(),
        path.c_str(),
        source.c_str(),
        diagnosticsBlob.writeRef()
    ));
    if (diagnosticsBlob) {
        std::cerr << (const char*)diagnosticsBlob->getBufferPointer() << std::endl;
    }

    // compile
    Slang::ComPtr<ISlangBlob> spirv;
    slangModule->getTargetCode(0, spirv.writeRef());

    // create vk shader module
    VkShaderModuleCreateInfo shaderModuleCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv->getBufferSize(),
        .pCode = (uint32_t*)spirv->getBufferPointer()
    };
    chk(vkCreateShaderModule(vk.ldevice.handle, &shaderModuleCI, nullptr, &shader));
}
