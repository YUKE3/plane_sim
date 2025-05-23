#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// Window dimensions
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

// Camera variables
float planeAltitude = 1.05f;  // Start just above the surface
float planeSpeed = 0.3f;      // Rotation speed (radians per second)
float planeAngle = 0.0f;      // Current angle around the globe
float planeTilt = 0.15f;      // Slight downward tilt to see the globe better
bool manualControl = false;   // Toggle for manual camera control

// Manual control variables
float cameraDistance = 3.0f;
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;
bool mousePressed = false;

// Globe rotation (for manual view)
float globeRotationX = 0.0f;
float globeRotationY = 0.0f;

// Vertex shader source
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
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

// Fragment shader source
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 sunPos;
uniform vec3 moonPos;
uniform vec3 sunColor;
uniform vec3 moonColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

// Simple noise function for continent generation
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(a, b, f.x) + (c - a) * f.y * (1.0 - f.x) + (d - b) * f.x * f.y;
}

float continentNoise(vec3 pos) {
    // Convert 3D position to 2D coordinates (like lat/long)
    float theta = atan(pos.z, pos.x);
    float phi = asin(pos.y);
    vec2 uv = vec2(theta * 2.0, phi * 3.0);
    
    // Layer multiple noise octaves for more realistic continents
    float n = 0.0;
    n += noise(uv * 3.0) * 0.5;
    n += noise(uv * 6.0) * 0.25;
    n += noise(uv * 12.0) * 0.125;
    
    // Create clearer land/water boundaries
    n = smoothstep(0.35, 0.45, n);
    
    return n;
}

void main() {
    vec3 norm = normalize(Normal);
    
    // Generate land/water based on position
    float landMask = continentNoise(normalize(FragPos));
    
    // Define colors
    vec3 oceanColor = vec3(0.05, 0.2, 0.5);   // Deep blue ocean
    vec3 landColor = vec3(0.15, 0.4, 0.15);   // Green land
    vec3 baseColor = mix(oceanColor, landColor, landMask);
    
    // Add more prominent desert regions
    if (landMask > 0.5) {
        // Create desert bands around certain latitudes (like Earth's desert belts)
        float latitude = abs(normalize(FragPos).y);
        float desertBelt = smoothstep(0.15, 0.25, latitude) * (1.0 - smoothstep(0.35, 0.45, latitude));
        
        // Add noise-based variation
        float variation = noise(normalize(FragPos).xz * 10.0);
        float desertAmount = desertBelt * 0.7 + variation * 0.3;
        
        vec3 desertColor = vec3(0.8, 0.6, 0.3);  // Sandy/orange desert
        vec3 savannaColor = vec3(0.5, 0.5, 0.2); // Yellowish savanna
        
        // Mix between green, savanna, and desert
        if (desertAmount > 0.3) {
            baseColor = mix(baseColor, savannaColor, smoothstep(0.3, 0.5, desertAmount));
        }
        if (desertAmount > 0.5) {
            baseColor = mix(baseColor, desertColor, smoothstep(0.5, 0.8, desertAmount));
        }
    }
    
    // Sun lighting (warm yellow)
    vec3 sunDir = normalize(sunPos - FragPos);
    float sunDiff = max(dot(norm, sunDir), 0.0);
    vec3 sunLight = sunDiff * sunColor * 0.8;
    
    // Moon lighting (cool blue-white)
    vec3 moonDir = normalize(moonPos - FragPos);
    float moonDiff = max(dot(norm, moonDir), 0.0);
    vec3 moonLight = moonDiff * moonColor * 0.3;
    
    // Ambient light (very dark, space-like)
    vec3 ambient = vec3(0.05, 0.05, 0.1);
    
    // Combine all lighting
    vec3 result = (ambient + sunLight + moonLight) * baseColor;
    
    // Add a subtle atmosphere glow at the edges
    vec3 viewDir = normalize(viewPos - FragPos);
    float rim = 1.0 - max(dot(norm, viewDir), 0.0);
    rim = pow(rim, 2.0);
    result += rim * vec3(0.1, 0.2, 0.4) * 0.5;
    
    // Add specular highlights on water
    if (landMask < 0.5) {
        vec3 halfwayDir = normalize(sunDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
        result += spec * sunColor * 0.5;
    }
    
    FragColor = vec4(result, 1.0);
}
)";

// Function to generate sphere vertices
void generateSphere(float radius, int sectors, int stacks,
                   std::vector<float>& vertices,
                   std::vector<unsigned int>& indices) {
    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / radius;
    float s, t;
    float sectorStep = 2 * M_PI / sectors;
    float stackStep = M_PI / stacks;
    float sectorAngle, stackAngle;

    // Generate vertices
    for(int i = 0; i <= stacks; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for(int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;

            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }

    // Generate indices
    int k1, k2;
    for(int i = 0; i < stacks; ++i) {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if(i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if(i != (stacks-1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

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
    if (!mousePressed || !manualControl) return;
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.01f;
    cameraAngleX += xoffset * sensitivity;
    cameraAngleY += yoffset * sensitivity;

    // Limit vertical rotation
    if (cameraAngleY > 1.5f) cameraAngleY = 1.5f;
    if (cameraAngleY < -1.5f) cameraAngleY = -1.5f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            firstMouse = true;
        } else if (action == GLFW_RELEASE) {
            mousePressed = false;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraDistance -= (float)yoffset * 0.1f;
    if (cameraDistance < 1.5f) cameraDistance = 1.5f;
    if (cameraDistance > 10.0f) cameraDistance = 10.0f;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Toggle manual camera control with SPACE
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spacePressed) {
            manualControl = !manualControl;
            if (manualControl) {
                std::cout << "Manual camera control enabled" << std::endl;
            } else {
                std::cout << "Plane view enabled" << std::endl;
            }
        }
        spacePressed = true;
    } else {
        spacePressed = false;
    }
    
    // Controls based on mode
    if (!manualControl) {
        // Plane view controls
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            planeSpeed += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            planeSpeed -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            planeAltitude -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            planeAltitude += 0.01f;
        
        // Clamp values
        if (planeSpeed < 0.0f) planeSpeed = 0.0f;
        if (planeSpeed > 2.0f) planeSpeed = 2.0f;
        if (planeAltitude < 1.05f) planeAltitude = 1.05f;
        if (planeAltitude > 2.0f) planeAltitude = 2.0f;
    } else {
        // Manual view - rotate globe with arrow keys
        float rotSpeed = 0.02f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            globeRotationY -= rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            globeRotationY += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            globeRotationX -= rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            globeRotationX += rotSpeed;
    }
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
    generateSphere(1.0f, 72, 36, vertices, indices);  // Doubled resolution for smoothness

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
        glm::mat4 model = glm::mat4(1.0f);
        
        // Apply globe rotation in manual mode
        if (manualControl) {
            model = glm::rotate(model, globeRotationY, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, globeRotationX, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        
        glm::mat4 view;
        if (!manualControl) {
            // Update plane position
            planeAngle += planeSpeed * deltaTime;
            
            // Create a figure-8 or sinusoidal path that varies in latitude
            float pathVariation = sin(planeAngle * 2.0f) * 0.4f; // Varies between -0.4 and +0.4
            
            // Calculate plane position with varying latitude
            float planeX = planeAltitude * cos(planeAngle);
            float planeY = pathVariation; // This makes the plane go up and down in latitude
            float planeZ = planeAltitude * sin(planeAngle);
            
            // Normalize and scale to maintain altitude
            glm::vec3 planePos = glm::vec3(planeX, planeY, planeZ);
            planePos = glm::normalize(planePos) * planeAltitude;
            
            // Calculate forward direction (tangent to the curved path)
            float nextAngle = planeAngle + 0.01f;
            float nextPathVariation = sin(nextAngle * 2.0f) * 0.4f;
            glm::vec3 nextPos = glm::normalize(glm::vec3(
                planeAltitude * cos(nextAngle),
                nextPathVariation,
                planeAltitude * sin(nextAngle)
            )) * planeAltitude;
            
            glm::vec3 forward = glm::normalize(nextPos - planePos);
            glm::vec3 up = glm::normalize(planePos);  // Up is away from globe center
            glm::vec3 right = glm::cross(forward, up);
            up = glm::cross(right, forward); // Recalculate up to ensure orthogonality
            
            // Add slight downward tilt to see the globe better
            glm::vec3 lookTarget = planePos + forward + up * (-planeTilt);
            
            view = glm::lookAt(planePos, lookTarget, up);
        } else {
            // Manual camera control
            float camX = sin(cameraAngleX) * cos(cameraAngleY) * cameraDistance;
            float camY = sin(cameraAngleY) * cameraDistance;
            float camZ = cos(cameraAngleX) * cos(cameraAngleY) * cameraDistance;
            
            view = glm::lookAt(
                glm::vec3(camX, camY, camZ),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
        }
        
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
        
        // Note: objectColor uniform is no longer used since colors are generated in shader
        
        // Camera/view position for rim lighting
        glm::vec3 viewPosition;
        if (!manualControl) {
            float pathVariation = sin(planeAngle * 2.0f) * 0.4f;
            viewPosition = glm::normalize(glm::vec3(
                planeAltitude * cos(planeAngle),
                pathVariation,
                planeAltitude * sin(planeAngle)
            )) * planeAltitude;
        } else {
            viewPosition = glm::vec3(
                sin(cameraAngleX) * cos(cameraAngleY) * cameraDistance,
                sin(cameraAngleY) * cameraDistance,
                cos(cameraAngleX) * cos(cameraAngleY) * cameraDistance
            );
        }
        glUniform3f(viewPosLoc, viewPosition.x, viewPosition.y, viewPosition.z);

        // Draw sphere
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}