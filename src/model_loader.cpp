#include "model_loader.h"
#include <iostream>
// Define implementations for tinygltf and stb_image
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

bool loadGLTFModel(const std::string& filepath, GLTFModel& model) {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    string err, warn;

    // Load the glTF file
    bool success = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filepath);
    if (!success) {
        std::cerr << "Failed to load glTF file: " << filepath << std::endl;
        std::cerr << "Error: " << err << "\nWarning: " << warn << std::endl;
        return false;
    }

    std::cout << "Successfully loaded glTF file: " << filepath << std::endl;

    // Additional code for processing the glTF model and preparing OpenGL buffers...
    return true;
}