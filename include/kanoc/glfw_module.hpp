#pragma once
#include <GLFW/glfw3.h>
#include <cstdint>
#include <vector>

#include "kanoc/devices.hpp"

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
