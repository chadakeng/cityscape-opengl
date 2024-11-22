cmake_minimum_required(VERSION 3.10)
project(OpenGL_GLTF_Loader)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find packages
find_package(glfw3 REQUIRED)       # For GLFW
find_package(OpenGL REQUIRED)      # For OpenGL
find_package(glm CONFIG REQUIRED)  # For GLM
find_package(TinyGLTF REQUIRED)    # For TinyGLTF (if available via package manager)

# If TinyGLTF is not found via package manager, you can include it manually
include_directories(${CMAKE_SOURCE_DIR}/path/to/tinygltf)

# Include directories
include_directories(
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/path/to/glad/include
)

# Add executable
add_executable(main main.cpp model_loader.cpp)

# Link libraries
target_link_libraries(main PRIVATE
    glfw
    ${OPENGL_gl_LIBRARY}
    glm::glm
    # Add TinyGLTF if it's a library; otherwise, it's header-only
)

# If using GLAD, include its source file
target_sources(main PRIVATE ${CMAKE_SOURCE_DIR}/path/to/glad/src/glad.c)
