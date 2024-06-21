#include "player.h"

Player::Player(float x, float y, float z) : posX(x), posY(y), posZ(z), velocityY(0.0f), rotationY(0.045f), velocityZ(0.0f), mass(1.0f) {}

void Player::updateVerticalPosition(float &deltaTime, float &heigh, float yaw)
{
    setRotY(yaw);
    float distanceToGround = rayCastGround(posX, posY, posZ, heigh);

    // avoid negative position, out of bounds
    if (posX < 0.1f)
    {
        posX = 0.1f;
    }
    if (posZ < 0.1f)
    {
        posZ = 0.1f;
    }

    // Update vertical position
    if (distanceToGround > 0.01f)
    {
        onAir = true;
    }
    if (onAir)
    {
        // Apply acceleration due to gravity
        velocityY -= gravity * deltaTime;
        posY += velocityY * deltaTime;
        float distanceToGround = rayCastGround(posX, posY, posZ, heigh);
        if (distanceToGround <= 0.01f)
        {
            onAir = false;
            velocityY = 0.0f;
        }
    }
    else
    {
        posY += (heigh - posY) * factor(deltaTime);
    }
}
void Player::run()
{
    velocity = 8.0f;
}
void Player::walk()
{
    velocity = 4.0f;
}
const float Player::getDirectionX()
{
    return cos(rotationY * invDegreesToRadians);
}
const float Player::getDirectionZ()
{
    return -sin(rotationY * invDegreesToRadians);
}
void Player::moveForward(float &deltatime)
{
    float movementSpeed = velocity * deltatime;
    posX += getDirectionZ() * movementSpeed;
    posZ -= getDirectionX() * movementSpeed;
}
void Player::moveBackwards(float &deltatime)
{
    float movementSpeed = velocity * deltatime;
    posX -= getDirectionZ() * movementSpeed;
    posZ += getDirectionX() * movementSpeed;
}
void Player::moveLeft(float &deltatime)
{
    float movementSpeed = velocity * deltatime;
    posX -= getDirectionX() * movementSpeed;
    posZ -= getDirectionZ() * movementSpeed;
}
void Player::moveRigth(float &deltatime)
{
    float movementSpeed = velocity * deltatime;
    posX += getDirectionX() * movementSpeed;
    posZ += getDirectionZ() * movementSpeed;
}
void Player::jump(float &deltatime)
{
    onAir = true;
    velocityY += velocity * 1.6;
}
void Player::setRotY(float &yaw)
{
    rotationY = -yaw;
}
const float Player::getPosX()
{
    return posX;
}
const float Player::getPosY()
{
    return posY;
}
const float Player::getPosZ()
{
    return posZ;
}
const float Player::getRotY()
{
    return rotationY;
}
const bool Player::isOnAir()
{
    return onAir > 0;
}
constexpr float Player::factor(float &deltaTime)
{
    return 1.0f - std::exp(-gravity * deltaTime);
}
const float Player::rayCastGround(float &x, float &y, float &z, float &heigh)
{
    for (float distance = 0.0f; distance <= maxDistance; distance += stepSize)
    {
        float currentX = x + rayDirectionX * distance;
        float currentY = y + rayDirectionY * distance;
        float currentZ = z + rayDirectionZ * distance;

        // Perform collision detection with the ground
        if (currentY <= heigh)
        {
            return distance;
        }
    }
    return maxDistance;
}
