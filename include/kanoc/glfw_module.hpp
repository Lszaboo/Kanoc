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

#include "cmd_helpers.hpp"
#include "devices.hpp"

class GLFWModule{
    public:
        GLFWwindow* window = nullptr;

        template<typename AppT>
        void Init(
            AppT* app,
            GLFWcursorposfun cursormovefun,
            GLFWmousebuttonfun cursorbuttonfun,
            GLFWkeyfun keyfun
        ){
            glfwInit();
            glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
            window = glfwCreateWindow( 1280/1.75f, 720/1.75f, "Vulkan window", nullptr, nullptr );
            assert(window);

            if (glfwRawMouseMotionSupported()){
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }else {
                // Check disabled because of MacOS!
                //CMD_PRINT("Raw cursor input not supported!");
                //exit(1);
            }

            glfwSetWindowUserPointer(window, app);
            glfwSetCursorPosCallback(window, cursormovefun);
            glfwSetMouseButtonCallback(window, cursorbuttonfun);
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

        void changeCursorVisinility(bool hidden){
            if (hidden) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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
};
