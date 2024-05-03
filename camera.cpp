// camera.cpp

#include "camera.h"

Camera::Camera(Character* target) : target(target) {
    // Initialize any member variables here
}

void Camera::update() {
    // Update the camera position and orientation based on the target character's position and orientation
    // For now, let's keep it simple and just set the camera position to the character's position
    float targetX = target->getX();
    float targetY = target->getY();
    float targetZ = target->getZ();

    // Update camera position
    // Example: setPosition(targetX, targetY + 5.0f, targetZ - 15.0f); // Adjust as needed
}
