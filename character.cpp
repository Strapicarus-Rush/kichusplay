// character.cpp

#include "character.h"
#include <cmath> // For trigonometric functions

Character::Character(const std::string& name, float health, float shield, float strength)
    : name(name), health(health), shield(shield), strength(strength) {}

void Character::setName(const std::string& name) {
    this->name = name;
}

std::string Character::getName() const {
    return name;
}

void Character::setHealth(float health) {
    this->health = health;
}

float Character::getHealth() const {
    return health;
}

void Character::setShield(float shield) {
    this->shield = shield;
}

float Character::getShield() const {
    return shield;
}

void Character::setStrength(float strength) {
    this->strength = strength;
}

float Character::getStrength() const {
    return strength;
}

//position functions
float Character::getX() const {
    return x;
}

float Character::getY() const {
    return y;
}

float Character::getZ() const {
    return z;
}

// Movement function implementation
void Character::move(float dx, float dy, float dz) {
    // Update character position accordingly
}

void Character::rotate(float angle) {
    // Rotate the character
    // For now, let's just rotate along the y-axis
    // Update character rotation accordingly
}
