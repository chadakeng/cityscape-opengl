// Include necessary headers
#include <iostream>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "tinygltf/stb_image.h"

// Constants for screen dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 80.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.1f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = -5.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float cameraSpeed = 20.0f; // Movement speed in world units/second

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Planets rotation
float planetRotation = 0.0f;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window, float deltaTime);
void generateSphere(float radius, unsigned int rings, unsigned int sectors,
    std::vector<float>& vertices, std::vector<unsigned int>& indices);
void checkShaderCompilation(GLuint shader);
void checkProgramLinking(GLuint program);

// Vertex Shader source code (with normals)
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos   = vec3(model * vec4(aPos, 1.0));
    Normal    = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

// Fragment Shader source code (with lighting calculations)
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    // Ambient lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * objectColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * objectColor;

    // Combine results
    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0);
}
)";

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System with Lighting", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Hide and capture the mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    checkShaderCompilation(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    checkShaderCompilation(fragmentShader);

    // Link shaders to create a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkProgramLinking(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate sphere data for sun and planets (with normals)
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(1.0f, 50, 50, sphereVertices, sphereIndices);

    // VAO and VBO for the spheres
    GLuint sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Generate stars data
    std::vector<float> starVertices;
    int numStars = 1000;
    for (int i = 0; i < numStars; ++i) {
        float x = ((rand() % 200) - 100) / 1.0f;
        float y = ((rand() % 200) - 100) / 1.0f;
        float z = ((rand() % 200) - 100) / 1.0f;
        starVertices.push_back(x);
        starVertices.push_back(y);
        starVertices.push_back(z);
    }

    // VAO and VBO for stars
    GLuint starsVAO, starsVBO;
    glGenVertexArrays(1, &starsVAO);
    glGenBuffers(1, &starsVBO);

    glBindVertexArray(starsVAO);

    glBindBuffer(GL_ARRAY_BUFFER, starsVBO);
    glBufferData(GL_ARRAY_BUFFER, starVertices.size() * sizeof(float), starVertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Scaling factors
    float distanceScale = 50.0f / 30.05f; // Scale Neptune's distance to 50 units

    // Planet data: scaled distance from sun, size, angular speed, color, tilt
    struct Planet {
        float distance;
        float size;
        float orbitSpeed;
        glm::vec3 color;
        float tilt;
    };

    std::vector<Planet> planets = {
        // Mercury
        {0.39f * distanceScale, 0.5f, 4.15f, glm::vec3(0.5f, 0.5f, 0.5f), 0.0f},
        // Venus
        {0.72f * distanceScale, 0.6f, 1.62f, glm::vec3(0.8f, 0.7f, 0.2f), 177.4f},
        // Earth
        {1.00f * distanceScale, 0.65f, 1.0f, glm::vec3(0.2f, 0.5f, 1.0f), 23.5f},
        // Mars
        {1.52f * distanceScale, 0.55f, 0.53f, glm::vec3(0.8f, 0.4f, 0.2f), 25.0f},
        // Jupiter
        {5.20f * distanceScale, 1.5f, 0.084f, glm::vec3(0.9f, 0.6f, 0.3f), 3.1f},
        // Saturn
        {9.58f * distanceScale, 1.2f, 0.034f, glm::vec3(0.9f, 0.8f, 0.5f), 26.7f},
        // Uranus
        {19.20f * distanceScale, 1.0f, 0.012f, glm::vec3(0.5f, 0.8f, 0.9f), 97.8f},
        // Neptune
        {30.05f * distanceScale, 0.95f, 0.006f, glm::vec3(0.3f, 0.5f, 0.9f), 28.3f},
    };

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Time calculations
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Update planet rotation (increase speed by 5 times)
        planetRotation += deltaTime * 5.0f;

        processInput(window, deltaTime);

        // Clear buffers
        glClearColor(0.0f, 0.0f, 0.02f, 1.0f); // Nearly black background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Set view and projection matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);

        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Set light and view positions
        glm::vec3 lightPos(0.0f, 0.0f, 0.0f); // Sun at origin
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

        // Draw stars
        glBindVertexArray(starsVAO);
        glm::mat4 model = glm::mat4(1.0f);
        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // White stars

        glPointSize(1.0f);
        glDrawArrays(GL_POINTS, 0, numStars);

        // Draw sun (emissive object)
        // For simplicity, we'll render the sun as a bright object without lighting calculations
        glBindVertexArray(sphereVAO);
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(5.0f)); // Increased sun size for visibility
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glUniform3f(colorLoc, 1.0f, 0.9f, 0.0f); // Yellow sun

        // Disable lighting for the sun
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(glm::vec3(0.0f)));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphereIndices.size()), GL_UNSIGNED_INT, 0);
        // Re-enable lighting
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));

        // Draw planets
        for (const auto& planet : planets) {
            model = glm::mat4(1.0f);
            float angle = planetRotation * planet.orbitSpeed;
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::translate(model, glm::vec3(planet.distance, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(planet.tilt), glm::vec3(0.0f, 0.0f, 1.0f)); // Planet tilt
            model = glm::scale(model, glm::vec3(planet.size * 1.5f)); // Adjusted planet size for visibility
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glUniform3fv(colorLoc, 1, glm::value_ptr(planet.color));

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphereIndices.size()), GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    glDeleteVertexArrays(1, &starsVAO);
    glDeleteBuffers(1, &starsVBO);

    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// Callback for resizing the window
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Mouse callback for camera rotation
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); // Reversed since y-coordinates go from bottom to top
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    float sensitivity = 0.1f; // Reduced mouse sensitivity
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Constrain the pitch to avoid flipping
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Update the camera direction
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// Process keyboard input
void processInput(GLFWwindow* window, float deltaTime) {
    float velocity = cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraFront * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraFront * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -=
        glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos +=
        glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;

    // Vertical movement
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraUp * velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraUp * velocity;

    // Escape to close the window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Function to generate sphere vertices and indices (with normals)
void generateSphere(float radius, unsigned int rings, unsigned int sectors,
    std::vector<float>& vertices,
    std::vector<unsigned int>& indices) {
    const float PI = 3.14159265359f;
    float const R = 1.0f / static_cast<float>(rings - 1);
    float const S = 1.0f / static_cast<float>(sectors - 1);

    for (unsigned int r = 0; r < rings; ++r) {
        for (unsigned int s = 0; s < sectors; ++s) {
            float y = sin(-PI / 2.0f + PI * r * R);
            float x = cos(2.0f * PI * s * S) * sin(PI * r * R);
            float z = sin(2.0f * PI * s * S) * sin(PI * r * R);

            // Position
            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);

            // Normal (normalized position vector)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    for (unsigned int r = 0; r < rings - 1; ++r) {
        for (unsigned int s = 0; s < sectors - 1; ++s) {
            indices.push_back(r * sectors + s);
            indices.push_back((r + 1) * sectors + s);
            indices.push_back((r + 1) * sectors + (s + 1));

            indices.push_back(r * sectors + s);
            indices.push_back((r + 1) * sectors + (s + 1));
            indices.push_back(r * sectors + (s + 1));
        }
    }
}

// Function to check shader compilation errors
void checkShaderCompilation(GLuint shader) {
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader Compilation Failed\n" << infoLog << std::endl;
    }
}

// Function to check program linking errors
void checkProgramLinking(GLuint program) {
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR: Program Linking Failed\n" << infoLog << std::endl;
    }
}
