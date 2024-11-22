#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include "tiny_gltf.h"
#include <glad/glad.h> // For OpenGL functions
#include <vector>
#include <string>
#include <iostream>
using std::cerr;
using std::string;

// Structure to hold OpenGL buffers for a model
struct GLTFModel {
    std::vector<GLuint> vbos; // Vertex Buffer Objects
    std::vector<GLuint> ebos; // Element Buffer Objects
    GLuint vao;               // Vertex Array Object
};

// Function to load a glTF model
bool loadGLTFModel(const std::string& filepath, GLTFModel& model);

#endif // MODEL_LOADER_H
