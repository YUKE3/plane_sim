#ifndef CAMERA_H
#define CAMERA_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

class Camera {
public:
    // Camera variables
    float planeAltitude = 1.08f;  // Slightly higher to avoid clipping
    float planeSpeed = 0.3f;      // Rotation speed (radians per second)
    float planeAngle = 0.0f;      // Current angle around the globe
    float planeTilt = 0.15f;      // Slight downward tilt to see the globe better
    bool manualControl = false;   // Toggle for manual camera control

    // Manual control variables
    float cameraDistance = 3.0f;
    float cameraAngleX = 0.0f;
    float cameraAngleY = 0.0f;
    float lastX;
    float lastY;
    bool firstMouse = true;
    bool mousePressed = false;

    // Globe rotation (for manual view)
    float globeRotationX = 0.0f;
    float globeRotationY = 0.0f;

    Camera(float windowWidth, float windowHeight) 
        : lastX(windowWidth / 2.0f), lastY(windowHeight / 2.0f) {}

    glm::mat4 getViewMatrix(float deltaTime) {
        if (!manualControl) {
            // Update plane position
            planeAngle += planeSpeed * deltaTime;
            
            // Create a figure-8 or sinusoidal path that varies in latitude
            float pathVariation = sin(planeAngle * 2.0f) * 0.4f;
            
            // Calculate plane position with varying latitude
            float planeX = planeAltitude * cos(planeAngle);
            float planeY = pathVariation;
            float planeZ = planeAltitude * sin(planeAngle);
            
            // Normalize and scale to maintain altitude
            glm::vec3 planePos = glm::vec3(planeX, planeY, planeZ);
            planePos = glm::normalize(planePos) * planeAltitude;
            
            // Calculate forward direction
            float nextAngle = planeAngle + 0.01f;
            float nextPathVariation = sin(nextAngle * 2.0f) * 0.4f;
            glm::vec3 nextPos = glm::normalize(glm::vec3(
                planeAltitude * cos(nextAngle),
                nextPathVariation,
                planeAltitude * sin(nextAngle)
            )) * planeAltitude;
            
            glm::vec3 forward = glm::normalize(nextPos - planePos);
            glm::vec3 up = glm::normalize(planePos);
            glm::vec3 right = glm::cross(forward, up);
            up = glm::cross(right, forward);
            
            glm::vec3 lookTarget = planePos + forward + up * (-planeTilt);
            
            return glm::lookAt(planePos, lookTarget, up);
        } else {
            // Manual camera control
            float camX = sin(cameraAngleX) * cos(cameraAngleY) * cameraDistance;
            float camY = sin(cameraAngleY) * cameraDistance;
            float camZ = cos(cameraAngleX) * cos(cameraAngleY) * cameraDistance;
            
            return glm::lookAt(
                glm::vec3(camX, camY, camZ),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
        }
    }

    glm::mat4 getModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);
        if (manualControl) {
            model = glm::rotate(model, globeRotationY, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, globeRotationX, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        return model;
    }

    glm::vec3 getViewPosition() {
        if (!manualControl) {
            float pathVariation = sin(planeAngle * 2.0f) * 0.4f;
            return glm::normalize(glm::vec3(
                planeAltitude * cos(planeAngle),
                pathVariation,
                planeAltitude * sin(planeAngle)
            )) * planeAltitude;
        } else {
            return glm::vec3(
                sin(cameraAngleX) * cos(cameraAngleY) * cameraDistance,
                sin(cameraAngleY) * cameraDistance,
                cos(cameraAngleX) * cos(cameraAngleY) * cameraDistance
            );
        }
    }

    void processKeyboard(GLFWwindow* window) {
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
            if (planeAltitude < 1.08f) planeAltitude = 1.08f;
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

    void processMouseMovement(double xpos, double ypos) {
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

        if (cameraAngleY > 1.5f) cameraAngleY = 1.5f;
        if (cameraAngleY < -1.5f) cameraAngleY = -1.5f;
    }

    void processMouseButton(int button, int action) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                mousePressed = true;
                firstMouse = true;
            } else if (action == GLFW_RELEASE) {
                mousePressed = false;
            }
        }
    }

    void processScroll(double yoffset) {
        cameraDistance -= (float)yoffset * 0.1f;
        if (cameraDistance < 1.5f) cameraDistance = 1.5f;
        if (cameraDistance > 10.0f) cameraDistance = 10.0f;
    }
};

#endif // CAMERA_H