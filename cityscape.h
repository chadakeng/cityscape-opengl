#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>


using std::vector;
using std::string;



struct Vertex {
    float x, y, z;
};

struct Normal {
    float x, y, z;
};

struct TexCoord {
    float u, v;
};
vector<Vertex> vertices;
vector<Normal> normals;
vector<TexCoord> texCoords;
vector<unsigned int> indices;

void processInput(GLFWwindow* window);