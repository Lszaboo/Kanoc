#include "kanoc/glfw_module.hpp"
#include <cassert>
#include <iostream>



void GLFWModule::Init(){
    glfwInit( );
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    window = glfwCreateWindow( 1280, 720, "Vulkan window", nullptr, nullptr );
    assert(window);
}

void GLFWModule::Destroy(){
    glfwDestroyWindow(window);
    glfwTerminate();
}

void GLFWModule::fillRequiredExtensions(Extensions& extensions){
    const char** strings = glfwGetRequiredInstanceExtensions(&extensions.count);
    for (uint32_t i=0; i<extensions.count; i++) {
        extensions.items.push_back(strings[i]);
    }
}

void GLFWModule::printWindowDimensions(){
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
