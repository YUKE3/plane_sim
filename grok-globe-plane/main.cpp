#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

// Vertex Shader Source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Fragment Shader Source
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main() {
    FragColor = vec4(color, 1.0);
}
)";

// Function to compile shaders
GLuint compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

// (Optional: Add generateSphere function here if you added it earlier)
// Function to generate a simple sphere (icosphere approximation)
std::vector<float> generateSphere(float radius, int sectors) {
    std::vector<float> vertices;
    float sectorStep = 2 * glm::pi<float>() / sectors;
    float stackStep = glm::pi<float>() / sectors;
    for (int i = 0; i <= sectors; ++i) {
        float stackAngle = glm::pi<float>() / 2 - i * stackStep;
        float xy = radius * cos(stackAngle);
        float z = radius * sin(stackAngle);
        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * sectorStep;
            float x = xy * cos(sectorAngle);
            float y = xy * sin(sectorAngle);
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
        }
    }
    // Add indices for triangles (simplified for brevity)
    // In a full implementation, generate indices for GL_TRIANGLES
    return vertices; // For now, use with GL_POINTS or expand to triangles
}


int main() {
    // Initialize GLFW and window setup (same as before)
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Plane Orbiting Planet", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW Error: " << glewGetErrorString(err) << std::endl;
    }
    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);

    // Shaders and program (same as before)
    GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::vector<float> planetVertices = generateSphere(0.5f, 20);
    std::vector<float> planeVertices = {
        0.0f, 0.1f, 0.0f,   -0.05f, -0.05f, 0.0f,   0.05f, -0.05f, 0.0f
    };

    // VAO/VBO setup for planet and plane (same as before)
    GLuint planetVAO, planetVBO;
    glGenVertexArrays(1, &planetVAO);
    glGenBuffers(1, &planetVBO);
    glBindVertexArray(planetVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planetVBO);
    glBufferData(GL_ARRAY_BUFFER, planetVertices.size() * sizeof(float), planetVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * sizeof(float), planeVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Rendering loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // Projection matrix (same)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Time for animation
        float time = glfwGetTime();

        // Plane position (orbiting)
        glm::vec3 planePos = glm::vec3(sin(time) * 1.5f, 0.2f, cos(time) * 1.5f); // Slight Y offset for visibility

        // Camera: Attached to plane, looking at planet (origin)
        glm::vec3 cameraPos = planePos + glm::vec3(0.0f, 0.2f, 0.5f); // Offset: a bit above and behind the plane
        glm::vec3 targetPos = glm::vec3(0.0f, 0.0f, 0.0f); // Look at planet center
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); // Up direction
        glm::mat4 view = glm::lookAt(cameraPos, targetPos, up);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // Draw Planet
        glm::mat4 planetModel = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(planetModel));
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.2f, 0.3f, 0.8f);
        glBindVertexArray(planetVAO);
        glDrawArrays(GL_TRIANGLES, 0, planetVertices.size() / 3);

        // Draw Plane (at its position)
        glm::mat4 planeModel = glm::translate(glm::mat4(1.0f), planePos);
        planeModel = glm::scale(planeModel, glm::vec3(0.2f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(planeModel));
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 0.2f, 0.2f);
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, planeVertices.size() / 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up (same as before)
    glDeleteVertexArrays(1, &planetVAO);
    glDeleteBuffers(1, &planetVBO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
