#pragma once
#include <GLFW/glfw3.h>
#include <cstdint>
#include <vector>

#include "vkwrappers/devices.hpp"

class GLFWModule{
    public:

        // fields
        GLFWwindow* window = nullptr;

        // functions
        void Init();
        void Destroy();
        void fillRequiredExtensions(Extensions& extensions);
        void printWindowDimensions();
};
