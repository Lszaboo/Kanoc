#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cassert>
#include <cstdint>
#include <glm/ext/vector_int2.hpp>
#include <vector>
#include <iostream>

#include "kanoc/cmd_helpers.hpp"
#include "kanoc/devices.hpp"

class GLFWModule{
    public:
        GLFWwindow* window = nullptr;

        template<typename AppT>
        void Init(
            AppT* app,
            GLFWcursorposfun cursorfun,
            GLFWkeyfun keyfun
        ){
            glfwInit();
            glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
            window = glfwCreateWindow( 1280, 720, "Vulkan window", nullptr, nullptr );
            assert(window);

            glfwSetWindowUserPointer(window, app);
            glfwSetCursorPosCallback(window, cursorfun);
            glfwSetKeyCallback(window, keyfun);
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

        glm::ivec2 getWindowDimension(){
            glm::ivec2 wsize;
            glfwGetFramebufferSize(window, &wsize.x, &wsize.y);
            return  wsize;
        }

        bool setWindowDimensionsIfDiffer(glm::ivec2& currWindowDimensions){
            glm::ivec2 glfwDimensions = getWindowDimension();

            if (currWindowDimensions != glfwDimensions) {
                currWindowDimensions = glfwDimensions;
                return true;
            }else {
                return false;
            }
        }

        glm::ivec2 getCorrectedCursorPos(glm::ivec2 windowDims, glm::vec2 rawCursorPos){
            glm::vec2 scale;
            glfwGetWindowContentScale(window, &scale.x, &scale.y);

            return glm::clamp(
                glm::ivec2(rawCursorPos)*glm::ivec2(scale),
                glm::ivec2(0,0),
                windowDims
            );
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
};
