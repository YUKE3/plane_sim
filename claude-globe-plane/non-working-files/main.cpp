#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "shaders.h"
#include "sphere.h"
#include "camera.h"

// Window dimensions
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

// Global camera object
Camera* camera = nullptr;

// Compile shader function
unsigned int compileShader(const char* source, GLenum type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    
    return shader;
}

// Callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (camera) camera->processMouseMovement(xpos, ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (camera) camera->processMouseButton(button, action);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (camera) camera->processScroll(yoffset);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (camera) camera->processKeyboard(window);
}

void printInstructions() {
    std::cout << "\n=== CONTROLS ===" << std::endl;
    std::cout << "SPACE: Toggle between plane view and manual camera" << std::endl;
    std::cout << "In Plane View:" << std::endl;
    std::cout << "  UP/DOWN: Increase/decrease speed" << std::endl;
    std::cout << "  LEFT/RIGHT: Decrease/increase altitude" << std::endl;
    std::cout << "In Manual Camera:" << std::endl;
    std::cout << "  Arrow Keys: Rotate the globe" << std::endl;
    std::cout << "  Mouse Drag: Move camera around globe" << std::endl;
    std::cout << "  Scroll: Zoom in/out" << std::endl;
    std::cout << "ESC: Exit\n" << std::endl;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Plane Around Planet", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    // Create camera
    camera = new Camera(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Compile shaders
    unsigned int vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);

    // Create shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check linking
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate sphere
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateSphere(1.0f, 72, 36, vertices, indices);

    // Create VAO, VBO, EBO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Get uniform locations
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int sunPosLoc = glGetUniformLocation(shaderProgram, "sunPos");
    int moonPosLoc = glGetUniformLocation(shaderProgram, "moonPos");
    int sunColorLoc = glGetUniformLocation(shaderProgram, "sunColor");
    int moonColorLoc = glGetUniformLocation(shaderProgram, "moonColor");
    int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

    // Print instructions
    printInstructions();

    // Render loop
    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Clear
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);

        // Set up matrices
        glm::mat4 model = camera->getModelMatrix();
        glm::mat4 view = camera->getViewMatrix(deltaTime);
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
            0.1f, 100.0f
        );

        // Set uniforms
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Sun and moon positions (opposite sides)
        glUniform3f(sunPosLoc, 5.0f, 0.0f, 0.0f);
        glUniform3f(moonPosLoc, -5.0f, 0.0f, 0.0f);
        
        // Light colors
        glUniform3f(sunColorLoc, 1.0f, 0.9f, 0.7f);  // Warm yellow
        glUniform3f(moonColorLoc, 0.7f, 0.8f, 1.0f); // Cool blue-white
        
        // Camera/view position for rim lighting
        glm::vec3 viewPosition = camera->getViewPosition();
        glUniform3f(viewPosLoc, viewPosition.x, viewPosition.y, viewPosition.z);

        // Draw sphere
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    delete camera;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}