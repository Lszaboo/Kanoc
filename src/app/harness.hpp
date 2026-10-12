#pragma once

#include "app/glfw_module.hpp"
#include "app/imgui_module.hpp"
#include "app/vk_module.hpp"

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

        void Init(){
            m.glfwmod.Init();
            m.vkmod.Init(m.glfwmod);
            m.guimod.Init(m.glfwmod, m.vkmod);

            app = new AppT();
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
