#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include "tiny_gltf.h"

// Declare global OpenGL buffers and tinygltf model
GLuint VAO, VBO, EBO;
tinygltf::Model model;


using std::vector;
using std::string;


// Structs to store vertex data
struct Vertex {
    float x, y, z;
};

struct Normal {
    float x, y, z;
};

struct TexCoord {
    float u, v;
};

// Vectors to store vertex data
vector<Vertex> vertices;
vector<Normal> normals;
vector<TexCoord> texCoords;
vector<unsigned int> indices;

// Function prototypes
void processInput(GLFWwindow* window);
bool loadModel(const string& filePath);
