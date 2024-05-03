// charactermovement.h

#pragma once

#include "character.h" // Include necessary headers

class CharacterMovement {
public:
    CharacterMovement(Character* character); // Constructor
    void moveForward(float speed);
    void moveBackward(float speed);
    void moveRight(float speed);
    void moveLeft(float speed);
    void rotate(float angle);
    void update(); // Declaration of the update function
private:
    Character* character; // Pointer to the character object
};
