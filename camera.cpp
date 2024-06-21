// camera.cpp

#include "camera.h"

Camera::Camera(float x, float y, float z) : posX(x), posY(y), posZ(z) {}

void Camera::updateCameraPosition(float targetPosX, float targetPosY, float targetPosZ, float &deltatime, float &cameraHeight)
{
    // Convert yaw from degrees to radians
    float yawRad = yaw * invDegreesToRadians;
    float pitchRad = maxHeight * cos(-pitch*invDegreesToRadians);
    // Calculate the direction from the camera to the targetPos
    viewX = targetPosX - posX;
    viewY = targetPosY - posY;
    viewZ = targetPosZ - posZ;

    // Normalize the direction
    float length = sqrt(viewX * viewX + viewY * viewY + viewZ * viewZ);
    viewX /= length;
    viewY /= length;
    viewZ /= length;

    // Calculate the desired position of the camera behind the target
    float targetX = targetPosX + viewX + distance * -std::sin(yawRad);
    float targetY = targetPosY + viewY + maxHeight;// std::min(maxHeight,std::max(cameraHeight+1.0f,pitchRad));
    float targetZ = targetPosZ + viewZ + distance * std::cos(yawRad);

    // Calculate the velocity of the camera towards the desired position
    // float factor = 1.0f - std::exp(-cameraSpeed * deltatime);

    // Update the camera position with the calculated velocity
    posX += (targetX - posX) * factor(deltatime);
    posY += (targetY - posY) * factor(deltatime);
    posZ += (targetZ - posZ) * factor(deltatime);
}
void Camera::updateRotation(float &yawMotion, float &pitchMotion)
{
    setYaw(yawMotion);
    setPitch(pitchMotion);
}
void Camera::setYaw(float &motion)
{
    yaw += motion;
    if (yaw > 360.0f)
    {
        yaw = 0.0f;
    }
    if (yaw < 0.0f)
    {
        yaw = 360.0f;
    }
}
void Camera::setPitch(float &motion)
{
    pitch += motion;
    if (pitch > 45.0f)
    {
        pitch = 45.0f;
    }
    if (pitch < -75.0f)
    {
        pitch = -75.0f;
    }
}
const float Camera::factor(float &deltaTime)
{
    return 1.0f - std::exp(-cameraSpeed * deltaTime);
}
const float Camera::getPosX()
{
    return posX;
}
const float Camera::getPosY()
{
    return posY;
}
const float Camera::getPosZ()
{
    return posZ;
}
const float Camera::getYaw()
{
    return yaw;
}
const float Camera::getPitch()
{
    return pitch;
}
const float Camera::getViewX()
{
    return viewX;
}
const float Camera::getViewY()
{
    return viewY;
}
const float Camera::getViewZ()
{
    return viewZ;
}
const float Camera::getRoll(){
    return roll;
}
