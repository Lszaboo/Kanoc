cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(DuckApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

macro(copy_to_build TRGT FLDR)
  add_custom_command(TARGET ${TRGT} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_SOURCE_DIR}/${FLDR}
        $<TARGET_FILE_DIR:${TRGT}>/${FLDR}
  )
endmacro()

# Kanoc include.
add_subdirectory(
  "${CMAKE_CURRENT_SOURCE_DIR}/../" # <- Location of Kanoc root!
  "${CMAKE_CURRENT_BINARY_DIR}/Kanoc" # <- Put build files into -B build folder!
)

# Find DuckApp *.cpp files.
file(GLOB_RECURSE SRC_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Assemble DuckApp *.cpp files.
add_executable(DuckApp
  ${SRC_FILES}
)

# Search for DuckApp headers here.
target_include_directories(DuckApp PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/src/"
)

# Link Kanoc with my DuckApp.
target_link_libraries(DuckApp
  PRIVATE
    Kanoc::Kanoc
)

# copy shaders next to DuckApp.
copy_to_build(DuckApp shaders)

# Copy assets next to DuckApp.
copy_to_build(DuckApp assets)
