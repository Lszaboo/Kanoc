#pragma once

#include "kanoc/glfw_module.hpp"
#include "kanoc/imgui_module.hpp"
#include "kanoc/vk_module.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <string>

/*
 * Harness for Apps.
 * The flow is:
 * Init -> Play -> Destroy
 */
template<typename AppT>
class Harness{
    public:
        struct Modules{
            GLFWModule glfwmod;
            VkModule vkmod;
            ImGuiModule guimod;
            std::string workingPath;
        };

        void Init(const std::string& argvzero){
            app = new AppT();
            auto dir = std::filesystem::absolute(argvzero).parent_path();
            m.workingPath = dir.string();

            m.glfwmod.Init(app);
            m.vkmod.Init(m.glfwmod);
            m.guimod.Init(m.glfwmod, m.vkmod, m.workingPath);

            app->Init(m);
        }

        void Destroy(){
            m.vkmod.WaitForIdle();

            app->Destroy(m);
            delete app;

            m.guimod.Destroy(m.vkmod);
            m.vkmod.Destroy();
            m.glfwmod.Destroy();
        }

        void Play(){
            app->Play(m);
        }

    protected:
        Harness::Modules m;
        AppT* app = nullptr;

};
