#pragma once

#include "kanoc/glfw_module.hpp"
#include "kanoc/imgui_module.hpp"
#include "kanoc/vk_module.hpp"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <filesystem>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <string>

class Modules{
    public:
        // Modules
        GLFWModule glfwmod;
        VkModule vkmod;
        ImGuiModule guimod;

        // App related general variables
        std::string workingPath;
        double time = 0.0;
        double oldTime = 0.0;
        double deltaTime = 0.0;


        template<typename AppT>
        void Init(AppT* app, const std::string& argvzero){
            auto dir = std::filesystem::absolute(argvzero).parent_path();
            workingPath = dir.string();

            glfwmod.Init(
                app,
                Modules::cursorPosCallback<AppT>,
                Modules::cursorClickCallback<AppT>,
                Modules::keyCallback<AppT>
            );

            vkmod.Init(glfwmod);
            guimod.Init(glfwmod, vkmod, workingPath);
        }

        void Destroy(){
            guimod.Destroy(vkmod);
            vkmod.Destroy();
            glfwmod.Destroy();
        }

        void updateDeltaTime(){
            oldTime = time;
            time = glfwGetTime();
            deltaTime = time - oldTime;
        }

    protected:
        template<typename AppT>
        static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos){
            auto app = static_cast<AppT*>(glfwGetWindowUserPointer(window));
            Modules* m = app->modules();

            glm::vec2 rawCursorPos = glm::vec2(xpos, ypos);
            glm::ivec2 correctedCursorPos = m->glfwmod.getCorrectedCursorPos(m->vkmod.windowDims, rawCursorPos);

            app->cursorPosChanged(rawCursorPos, correctedCursorPos, m->guimod.io->WantCaptureMouse);
        }

        template<typename AppT>
        static void cursorClickCallback(GLFWwindow* window, int button, int action, int mods)
        {
            auto app = static_cast<AppT*>(glfwGetWindowUserPointer(window));

            if (action == GLFW_PRESS){
                app->cursorButtonPressed(button);
            }else if (action == GLFW_RELEASE) {
                app->cursorButtonReleased(button);
            }
        }

        template<typename AppT>
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
            auto app = static_cast<AppT*>(glfwGetWindowUserPointer(window));

            if (action == GLFW_PRESS){
                app->keyPressed(key);
            }else if (action == GLFW_RELEASE) {
                app->keyReleased(key);
            }
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
