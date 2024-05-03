// character.h

#ifndef CHARACTER_H
#define CHARACTER_H

#include <string>

class Character {
public:
    Character(const std::string& name, float health, float shield, float strength);

    void setName(const std::string& name);
    std::string getName() const;

    void setHealth(float health);
    float getHealth() const;

    void setShield(float shield);
    float getShield() const;

    void setStrength(float strength);
    float getStrength() const;

    //position functions
    float getX() const;
    float getY() const;
    float getZ() const;

    // Movement functions
    void move(float dx, float dy, float dz);

    void rotate(float angle);

private:
    std::string name;
    float health;
    float shield;
    float strength;
    // Add more member variables as needed
};

#endif // CHARACTER_H
