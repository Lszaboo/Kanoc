#pragma once

#include "kanoc/glfw_module.hpp"
#include "kanoc/imgui_module.hpp"
#include "kanoc/vk_module.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <string>

class Modules{
    public:
        GLFWModule glfwmod;
        VkModule vkmod;
        ImGuiModule guimod;
        std::string workingPath;

        template<typename AppT>
        void Init(AppT* app, const std::string& argvzero){
            auto dir = std::filesystem::absolute(argvzero).parent_path();
            workingPath = dir.string();

            glfwmod.Init(app);
            vkmod.Init(glfwmod);
            guimod.Init(glfwmod, vkmod, workingPath);
        }

        void Destroy(){
            guimod.Destroy(vkmod);
            vkmod.Destroy();
            glfwmod.Destroy();
        }
};

/*
 * Harness for Apps.
 * The flow is:
 * Init -> Play -> Destroy
 */
template<typename AppT>
class Harness{
    public:
        AppT* app = nullptr;
        Modules* modules = nullptr;

        void Init(const std::string& argvzero){
            app = new AppT();
            modules = new Modules();

            modules->Init(app, argvzero);
            app->Init(modules);
        }

        void Play(){
            app->Play();
        }

        void Destroy(){
            modules->vkmod.WaitForIdle();

            app->Destroy();
            delete app;

            modules->Destroy();
            delete modules;
        }
};
