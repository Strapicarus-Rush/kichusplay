 charactermovement.cpp

#include "charactermovement.h"

CharacterMovement::CharacterMovement(Character* character) : character(character) {
    // Initialize any member variables or setup needed
}

void CharacterMovement::moveForward(float speed) {
    // Move the character forward along its facing direction
    // For now, let's assume the character faces along the positive z-axis
    character->move(0.0f, 0.0f, speed);
}

void CharacterMovement::moveBackward(float speed) {
    // Move the character backward along its facing direction
    // For now, let's assume the character faces along the positive z-axis
    character->move(0.0f, 0.0f, -speed);
}

void CharacterMovement::moveRight(float speed) {
    // Move the character to the right (relative to its facing direction)
    // For now, let's assume the character faces along the positive z-axis
    character->move(speed, 0.0f, 0.0f);
}

void CharacterMovement::moveLeft(float speed) {
    // Move the character to the left (relative to its facing direction)
    // For now, let's assume the character faces along the positive z-axis
    character->move(-speed, 0.0f, 0.0f);
}

void CharacterMovement::rotate(float angle) {
    // Rotate the character
    // For now, let's just rotate along the y-axis
    character->rotate(angle);
}

void CharacterMovement::update() {
    // Implementation of the update function
    // Add your code here to update the character movement
}
