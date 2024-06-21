#ifndef PLAYER_H
#define PLAYER_H
#include <cmath>

extern bool debug;
extern const float invDegreesToRadians;

class Player
{
public:
    Player(float x, float y, float z);
    void updateVerticalPosition(float &deltaTime, float &heigh, float yaw);
    void run();
    void walk();
    void moveForward(float &deltatime);
    void moveBackwards(float &deltatime);
    void moveLeft(float &deltatime);
    void moveRigth(float &deltatime);
    void jump(float &deltatime);
    void setRotY(float &yaw);
    const float getPosX();
    const float getPosY();
    const float getPosZ();
    const float getRotY();
    const bool isOnAir();

private:
    constexpr float factor(float &deltaTime);
    static const float rayCastGround(float &x, float &y, float &z, float &heigh);
    static const constexpr float rayDirectionX = 0.0f;
    static const constexpr float rayDirectionY = -1.0f;
    static const constexpr float rayDirectionZ = 0.0f;
    static const constexpr float stepSize = 0.001f;
    static const constexpr float maxDistance = 2.0f;
    static const constexpr float gravity = 9.8f;
    float velocity = 14.0f;
    float posX;
    float posY;
    float posZ;
    float velocityX;
    float velocityY;
    float velocityZ;
    float rotationY;
    float mass;
    const float getDirectionX();
    const float getDirectionZ();
    bool onAir = true;
};
#endif // PLAYER_H