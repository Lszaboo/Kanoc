#pragma once

#include "kanoc/glfw_module.hpp"
#include "kanoc/imgui_module.hpp"
#include "kanoc/vk_module.hpp"
#include <filesystem>

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
        };

        void Init(const std::string& argvzero){
            auto dir = std::filesystem::absolute(argvzero).parent_path();
            std::string workingPath = dir.string();

            m.glfwmod.Init();
            m.vkmod.Init(m.glfwmod);

            app = new AppT();

            glfwSetWindowUserPointer(m.glfwmod.window, app);
            glfwSetCursorPosCallback(m.glfwmod.window, AppT::cursorPosChanged);
            m.guimod.Init(m.glfwmod, m.vkmod, workingPath);

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
