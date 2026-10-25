#pragma once

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>

#include "kanoc/devices.hpp"

class GLFWModule{
    public:
        GLFWwindow* window = nullptr;

        template<typename AppT>
        void Init(AppT* app){
            glfwInit();
            glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
            window = glfwCreateWindow( 1280, 720, "Vulkan window", nullptr, nullptr );
            assert(window);

            glfwSetWindowUserPointer(window, app);
            glfwSetCursorPosCallback(window, GLFWModule::cursorPosCallback<AppT>);
        }

        void Destroy(){
            glfwDestroyWindow(window);
            glfwTerminate();
        }

        void fillRequiredExtensions(Extensions& extensions){
            const char** strings = glfwGetRequiredInstanceExtensions(&extensions.count);
            for (uint32_t i=0; i<extensions.count; i++) {
                extensions.items.push_back(strings[i]);
            }
        }

        void printWindowDimensions(){
            int windowWidth, windowHeight;
            int framebufferWidth, framebufferHeight;
            float scaleX, scaleY;

            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            glfwGetWindowContentScale(window, &scaleX, &scaleY);

            std::cout << "window: " << windowWidth << "x" << windowHeight << std::endl;
            std::cout << "framebuffer: " << framebufferWidth << "x" << framebufferHeight << std::endl;
            std::cout << "scale: " << scaleX << "x" << scaleY << std::endl;
        }

    protected:

        template<typename AppT>
        static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos){
            auto app = static_cast<AppT*>(glfwGetWindowUserPointer(window));
            app->cursorPosChanged(xpos, ypos);
        }
};
