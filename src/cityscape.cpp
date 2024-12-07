// Include necessary headers
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

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
glm::vec3 cameraPos = glm::vec3(-1200.0f, 200.0f, 0.0f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(1.0f, -0.1f, 0.0f)); // Looking towards positive x-axis
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = 0.0f;
float pitch = -5.0f; // Slight downward angle
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float cameraSpeed = 200.0f; // Increased movement speed

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Planets rotation
float planetRotation = 0.0f; // Start with planets on the same side

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window, float deltaTime);
void generateSphere(float radius, unsigned int rings, unsigned int sectors,
    std::vector<float>& vertices, std::vector<unsigned int>& indices);
void checkShaderCompilation(GLuint shader);
void checkProgramLinking(GLuint program);
void generateCircle(float radius, int segments, std::vector<float>& vertices);
unsigned int loadTexture(const std::string& path);
void generateRing(float innerRadius, float outerRadius, int segments,
    std::vector<float>& vertices, std::vector<unsigned int>& indices);

// --------------------- Added ring generation function -----------------------
void generateRing(float innerRadius, float outerRadius, int segments,
    std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    float angleIncrement = 2.0f * glm::pi<float>() / segments;

    for (int i = 0; i <= segments; ++i) {
        float angle = i * angleIncrement;
        float x = cos(angle);
        float z = sin(angle);

        // Outer vertex position
        float xOuter = x * outerRadius;
        float zOuter = z * outerRadius;
        // Inner vertex position
        float xInner = x * innerRadius;
        float zInner = z * innerRadius;

        // Normal for both inner and outer vertices is up (0,1,0)
        float nx = 0.0f;
        float ny = 1.0f;
        float nz = 0.0f;

        // Outer vertex
        vertices.push_back(xOuter);
        vertices.push_back(0.0f);
        vertices.push_back(zOuter);
        vertices.push_back(nx);
        vertices.push_back(ny);
        vertices.push_back(nz);

        // Inner vertex
        vertices.push_back(xInner);
        vertices.push_back(0.0f);
        vertices.push_back(zInner);
        vertices.push_back(nx);
        vertices.push_back(ny);
        vertices.push_back(nz);
    }

    // Create indices for a triangle strip
    for (int i = 0; i < segments; ++i) {
        int start = i * 2;
        indices.push_back(start);
        indices.push_back(start + 1);
        indices.push_back(start + 2);

        indices.push_back(start + 1);
        indices.push_back(start + 3);
        indices.push_back(start + 2);
    }
}
// ---------------------------------------------------------------------------

// Planet Vertex Shader
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos   = vec3(model * vec4(aPos, 1.0));
    Normal    = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

// Planet Fragment Shader
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    // Ambient lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * texture(texture1, TexCoords).rgb;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * texture(texture1, TexCoords).rgb;

    // Combine results
    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0);
}
)";

// Sun Vertex Shader (using texture coords)
const char* sunVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal; // unused but must be declared since sphere data includes it
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Sun Fragment Shader (animated "boiling" effect)
const char* sunFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sunTexture;
uniform float time;

void main() {
    // Distortion parameters for "boiling"
    float distortionStrength = 0.02;
    float uOffset = sin(time * 0.5 + TexCoords.t * 10.0) * distortionStrength;
    float vOffset = cos(time * 0.7 + TexCoords.s * 10.0) * distortionStrength;
    vec2 distortedUV = TexCoords + vec2(uOffset, vOffset);

    // The sun is emissive
    FragColor = texture(sunTexture, distortedUV);
}
)";

// Star Vertex Shader
const char* starVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

// Star Fragment Shader
const char* starFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 starColor;

void main() {
    FragColor = vec4(starColor, 1.0);
}
)";

// Orbit Vertex Shader
const char* orbitVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Orbit Fragment Shader
const char* orbitFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 orbitColor;

void main() {
    FragColor = vec4(orbitColor, 1.0);
}
)";

struct Planet {
    float distance;            // Scaled distance from the sun
    float size;                // Scaled size of the planet
    float orbitSpeed;          // Angular speed of orbit
    glm::vec3 color;           // Base color (used as fallback)
    float tilt;                // Axial tilt
    unsigned int textureID;    // OpenGL texture ID
    GLuint orbitVAO;           // VAO for orbit path
    GLuint orbitVBO;           // VBO for orbit path
    int orbitVertexCount;      // Number of vertices in orbit path
    std::string texturePath;   // Path to the texture image

    Planet(float dist, float sz, float orbSpeed, const glm::vec3& col, float tl, const std::string& texPath)
        : distance(dist), size(sz), orbitSpeed(orbSpeed), color(col), tilt(tl),
        textureID(0), orbitVAO(0), orbitVBO(0), orbitVertexCount(0),
        texturePath(texPath) {}
};

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System with Boiling Sun Texture", nullptr, nullptr);
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

    // Compile shaders for planets
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    checkShaderCompilation(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    checkShaderCompilation(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkProgramLinking(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Compile shaders for sun
    GLuint sunVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sunVertexShader, 1, &sunVertexShaderSource, nullptr);
    glCompileShader(sunVertexShader);
    checkShaderCompilation(sunVertexShader);

    GLuint sunFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sunFragmentShader, 1, &sunFragmentShaderSource, nullptr);
    glCompileShader(sunFragmentShader);
    checkShaderCompilation(sunFragmentShader);

    GLuint sunShaderProgram = glCreateProgram();
    glAttachShader(sunShaderProgram, sunVertexShader);
    glAttachShader(sunShaderProgram, sunFragmentShader);
    glLinkProgram(sunShaderProgram);
    checkProgramLinking(sunShaderProgram);
    glDeleteShader(sunVertexShader);
    glDeleteShader(sunFragmentShader);

    // Compile shaders for stars
    GLuint starVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(starVertexShader, 1, &starVertexShaderSource, nullptr);
    glCompileShader(starVertexShader);
    checkShaderCompilation(starVertexShader);

    GLuint starFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(starFragmentShader, 1, &starFragmentShaderSource, nullptr);
    glCompileShader(starFragmentShader);
    checkShaderCompilation(starFragmentShader);

    GLuint starShaderProgram = glCreateProgram();
    glAttachShader(starShaderProgram, starVertexShader);
    glAttachShader(starShaderProgram, starFragmentShader);
    glLinkProgram(starShaderProgram);
    checkProgramLinking(starShaderProgram);
    glDeleteShader(starVertexShader);
    glDeleteShader(starFragmentShader);

    // Compile shaders for orbits
    GLuint orbitVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(orbitVertexShader, 1, &orbitVertexShaderSource, nullptr);
    glCompileShader(orbitVertexShader);
    checkShaderCompilation(orbitVertexShader);

    GLuint orbitFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(orbitFragmentShader, 1, &orbitFragmentShaderSource, nullptr);
    glCompileShader(orbitFragmentShader);
    checkShaderCompilation(orbitFragmentShader);

    GLuint orbitShaderProgram = glCreateProgram();
    glAttachShader(orbitShaderProgram, orbitVertexShader);
    glAttachShader(orbitShaderProgram, orbitFragmentShader);
    glLinkProgram(orbitShaderProgram);
    checkProgramLinking(orbitShaderProgram);
    glDeleteShader(orbitVertexShader);
    glDeleteShader(orbitFragmentShader);

    // Generate sphere data
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(1.0f, 50, 50, sphereVertices, sphereIndices);

    // VAO and VBO for spheres
    GLuint sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    // Sphere attributes: position, normal, texcoords
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Generate stars data (expanded range)
    std::vector<float> starVertices;
    int numStars = 8000;
    for (int i = 0; i < numStars; ++i) {
        float x = ((rand() % 8000) - 4000) / 1.0f; // Twice the previous range
        float y = ((rand() % 8000) - 4000) / 1.0f;
        float z = ((rand() % 8000) - 4000) / 1.0f;
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Scaling factors
    float distanceScale = 2000.0f / 30.05f; // Spaces out planets further

    // Planets definition
    std::vector<Planet> planets = {
        Planet(0.39f * distanceScale, 0.2f, 3.2f, glm::vec3(0.7f, 0.7f, 0.7f), 0.034f, "textures/mercury.jpg"),
        Planet(0.72f * distanceScale, 0.3f, 2.3f, glm::vec3(0.9f, 0.7f, 0.3f), 177.4f, "textures/venus.jpg"),
        Planet(1.00f * distanceScale, 0.4f, 2.0f, glm::vec3(0.2f, 0.5f, 1.0f), 23.5f, "textures/earth.jpg"),
        Planet(1.52f * distanceScale, 0.24f, 1.6f, glm::vec3(0.8f, 0.3f, 0.2f), 25.0f, "textures/mars.jpg"),
        Planet(5.20f * distanceScale, 1.2f, 0.8f, glm::vec3(0.9f, 0.6f, 0.3f), 3.1f, "textures/jupiter.jpg"),
        Planet(9.58f * distanceScale, 1.0f, 0.64f, glm::vec3(0.9f, 0.8f, 0.5f), 26.7f, "textures/saturn.jpg"),
        Planet(19.20f * distanceScale, 0.45f, 0.45f, glm::vec3(0.5f, 0.8f, 0.9f), 97.8f, "textures/uranus.jpg"),
        Planet(30.05f * distanceScale, 0.4f, 0.36f, glm::vec3(0.3f, 0.5f, 0.9f), 28.3f, "textures/neptune.jpg")
    };

    // Load textures for each planet
    for (auto& planet : planets) {
        planet.textureID = loadTexture(planet.texturePath);
        if (planet.textureID == 0) {
            std::cerr << "Failed to load texture for planet: " << planet.texturePath << std::endl;
        }
    }

    // Generate orbit paths for planets
    for (auto& planet : planets) {
        std::vector<float> orbitVertices;
        int segments = 200;
        generateCircle(planet.distance, segments, orbitVertices);
        planet.orbitVertexCount = segments;

        glGenVertexArrays(1, &planet.orbitVAO);
        glGenBuffers(1, &planet.orbitVBO);

        glBindVertexArray(planet.orbitVAO);

        glBindBuffer(GL_ARRAY_BUFFER, planet.orbitVBO);
        glBufferData(GL_ARRAY_BUFFER, orbitVertices.size() * sizeof(float), orbitVertices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    // Generate ring for Saturn
    float ringInnerRadius = 12.0f;
    float ringOuterRadius = 20.0f;
    int ringSegments = 100;
    std::vector<float> ringVertices;
    std::vector<unsigned int> ringIndices;

    generateRing(ringInnerRadius, ringOuterRadius, ringSegments, ringVertices, ringIndices);

    GLuint ringVAO, ringVBO, ringEBO;
    glGenVertexArrays(1, &ringVAO);
    glGenBuffers(1, &ringVBO);
    glGenBuffers(1, &ringEBO);

    glBindVertexArray(ringVAO);

    glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
    glBufferData(GL_ARRAY_BUFFER, ringVertices.size() * sizeof(float), ringVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ringEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ringIndices.size() * sizeof(unsigned int), ringIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Load sun texture
    unsigned int sunTextureID = loadTexture("textures/sun.jpg");
    if (sunTextureID == 0) {
        std::cerr << "Failed to load sun texture." << std::endl;
    }

    float globalOrbitSpeedFactor = 0.05f;
    // Slower planet self-rotation factor
    float globalSelfRotationSpeedFactor = 0.1f; // Reduced from 5.0f to slow down

    glEnable(GL_DEPTH_TEST);

    // Configure shader program for planets
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // Configure shader program for sun
    glUseProgram(sunShaderProgram);
    glUniform1i(glGetUniformLocation(sunShaderProgram, "sunTexture"), 0);

    // We'll also need the time uniform for the sun
    GLint sunTimeLoc = glGetUniformLocation(sunShaderProgram, "time");

    // Sun rotation speed
    float sunRotationSpeed = 5.0f; // degrees per second

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        planetRotation += deltaTime * 1.0f;

        processInput(window, deltaTime);

        glClearColor(0.0f, 0.0f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(60.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);

        // Draw stars
        glUseProgram(starShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(starShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(starShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(starShaderProgram, "starColor"), 1.0f, 1.0f, 1.0f);

        glBindVertexArray(starsVAO);
        glPointSize(2.0f);
        glDrawArrays(GL_POINTS, 0, numStars);

        // Draw orbits
        glUseProgram(orbitShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(orbitShaderProgram, "orbitColor"), 1.0f, 1.0f, 1.0f);

        for (const auto& planet : planets) {
            glm::mat4 orbitModel = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(orbitModel));
            glBindVertexArray(planet.orbitVAO);
            glDrawArrays(GL_LINE_LOOP, 0, planet.orbitVertexCount);
        }

        // Draw sun (with animation)
        glUseProgram(sunShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(sunShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(sunShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 sunModel = glm::mat4(1.0f);
        // Rotate the sun over time
        sunModel = glm::rotate(sunModel, glm::radians(sunRotationSpeed * currentFrame), glm::vec3(0.0f, 1.0f, 0.0f));
        sunModel = glm::scale(sunModel, glm::vec3(20.0f));
        glUniformMatrix4fv(glGetUniformLocation(sunShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(sunModel));

        // Pass time for the boiling effect
        glUniform1f(sunTimeLoc, currentFrame);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sunTextureID);

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphereIndices.size()), GL_UNSIGNED_INT, 0);

        // Draw planets
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(glm::vec3(0.0f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
        int modelLoc = glGetUniformLocation(shaderProgram, "model");

        float sizeMultiplier = 10.0f;
        glm::mat4 saturnModel;

        for (size_t i = 0; i < planets.size(); ++i) {
            const auto& planet = planets[i];
            glm::mat4 model = glm::mat4(1.0f);
            float angle = planetRotation * planet.orbitSpeed * globalOrbitSpeedFactor;
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::translate(model, glm::vec3(planet.distance, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(planet.tilt), glm::vec3(0.0f, 0.0f, 1.0f));

            // Slower self-rotation
            float rotationAngle = currentFrame * planet.orbitSpeed * globalSelfRotationSpeedFactor;
            model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(planet.size * sizeMultiplier));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, planet.textureID);

            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphereIndices.size()), GL_UNSIGNED_INT, 0);

            // Save Saturn's model without the final self-rotation for the ring alignment
            if (i == 5) {
                glm::mat4 saturnBaseModel = glm::mat4(1.0f);
                saturnBaseModel = glm::rotate(saturnBaseModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));
                saturnBaseModel = glm::translate(saturnBaseModel, glm::vec3(planet.distance, 0.0f, 0.0f));
                saturnBaseModel = glm::rotate(saturnBaseModel, glm::radians(planet.tilt), glm::vec3(0.0f, 0.0f, 1.0f));
                saturnBaseModel = glm::scale(saturnBaseModel, glm::vec3(sizeMultiplier));
                saturnModel = saturnBaseModel;
            }
        }

        // Draw Saturn's ring
        glUseProgram(orbitShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(saturnModel));
        glUniform3f(glGetUniformLocation(orbitShaderProgram, "orbitColor"), 1.0f, 1.0f, 1.0f);

        glBindVertexArray(ringVAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ringIndices.size()), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    glDeleteVertexArrays(1, &starsVAO);
    glDeleteBuffers(1, &starsVBO);

    for (const auto& planet : planets) {
        glDeleteVertexArrays(1, &planet.orbitVAO);
        glDeleteBuffers(1, &planet.orbitVBO);
        glDeleteTextures(1, &planet.textureID);
    }

    glDeleteVertexArrays(1, &ringVAO);
    glDeleteBuffers(1, &ringVBO);
    glDeleteBuffers(1, &ringEBO);

    glDeleteProgram(shaderProgram);
    glDeleteProgram(sunShaderProgram);
    glDeleteProgram(starShaderProgram);
    glDeleteProgram(orbitShaderProgram);

    glDeleteTextures(1, &sunTextureID);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow* window, float deltaTime) {
    float velocity = cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraFront * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraFront * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraUp * velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraUp * velocity;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void generateSphere(float radius, unsigned int rings, unsigned int sectors,
    std::vector<float>& vertices,
    std::vector<unsigned int>& indices) {
    const float PI = 3.14159265359f;
    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    for (unsigned int r = 0; r < rings; ++r) {
        for (unsigned int s = 0; s < sectors; ++s) {
            float y = sin(-PI / 2.0f + PI * r * R);
            float x = cos(2.0f * PI * s * S) * sin(PI * r * R);
            float z = sin(2.0f * PI * s * S) * sin(PI * r * R);

            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            float u = (float)s / (float)(sectors - 1);
            float v = (float)r / (float)(rings - 1);
            vertices.push_back(u);
            vertices.push_back(v);
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

void generateCircle(float radius, int segments, std::vector<float>& vertices) {
    const float PI = 3.14159265359f;
    float angleIncrement = 2.0f * PI / segments;
    for (int i = 0; i < segments; ++i) {
        float angle = i * angleIncrement;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);
    }
}

void checkShaderCompilation(GLuint shader) {
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader Compilation Failed\n" << infoLog << std::endl;
    }
}

void checkProgramLinking(GLuint program) {
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR: Program Linking Failed\n" << infoLog << std::endl;
    }
}

unsigned int loadTexture(const std::string& path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
