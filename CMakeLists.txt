cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Kanoc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
  list(APPEND CMAKE_PREFIX_PATH "$ENV{VK_CMAKE_LOC}")
else()
  list(APPEND CMAKE_PREFIX_PATH "$ENV{VULKAN_SDK}")
endif()

# find packages from OS
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Ktx REQUIRED)

# find pacgakes from VULKAN_SDK
find_package(slang CONFIG REQUIRED)

include(FetchContent)

macro(setup_imgui)
  # pull imgui from github
  FetchContent_Declare(
      imgui
      GIT_REPOSITORY https://github.com/ocornut/imgui.git
      GIT_TAG v1.92.9b
  )
  FetchContent_MakeAvailable(imgui)

  # assemble imgui files
  add_library(imgui STATIC
      ${imgui_SOURCE_DIR}/imgui.cpp
      ${imgui_SOURCE_DIR}/imgui_draw.cpp
      ${imgui_SOURCE_DIR}/imgui_tables.cpp
      ${imgui_SOURCE_DIR}/imgui_widgets.cpp
      # GLFW + Vulkan backend
      ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
      ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  )

  # include imgui in build
  target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
  )

  # link imgui with vulkan and glfw
  target_link_libraries(imgui PUBLIC
      Vulkan::Vulkan
      glfw
  )

  # finally, add imgui
  add_library(imgui::imgui ALIAS imgui)
endmacro()

macro(setup_vma)
  # pull VMA from github
  FetchContent_Declare(
      VulkanMemoryAllocator
      GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
      GIT_TAG v3.3.0
  )
  FetchContent_MakeAvailable(VulkanMemoryAllocator)
endmacro()

macro(setup_tinyobjloader)
  # pull tinyobjloader from github
  FetchContent_Declare(
      tinyobjloader
      GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
      GIT_TAG v2.0.0rc13
  )
  FetchContent_MakeAvailable(tinyobjloader)
endmacro()

# macro(copy_to_build TRGT FLDR)
#   add_custom_command(TARGET ${TRGT} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
#         ${CMAKE_SOURCE_DIR}/${FLDR}
#         $<TARGET_FILE_DIR:${TRGT}>/${FLDR}
#   )
# endmacro()

setup_imgui()
setup_vma()
setup_tinyobjloader()

# Find library *.cpp files in src/
file(GLOB_RECURSE SRC_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Assemble library *.cpp files.
add_library(Kanoc STATIC
  ${SRC_FILES}
)

# Search for library headers here.
target_include_directories(Kanoc PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

# Link the library with it's dependencies.
target_link_libraries(Kanoc PUBLIC
    Vulkan::Vulkan
    glm::glm
    glfw
    imgui::imgui
    GPUOpen::VulkanMemoryAllocator
    tinyobjloader
    ktx
    slang::slang
)

add_library(Kanoc::Kanoc ALIAS Kanoc)

# copy shaders next to app
# copy_to_build(VulkanApp shaders)

# copy assets next to app
# copy_to_build(VulkanApp assets)
