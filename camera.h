// camera.h

#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
extern bool debug;
extern const float invRadiansToDegrees;
extern const float invDegreesToRadians;
extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;
class Camera
{
public:
    Camera(float x, float y, float z);
    void updateCameraPosition(float targetX, float targetY, float targetZ, float &deltatime, float &height);
    void updateRotation(float &yawMotion, float &pitchMotion);
    static const constexpr float fov = 60.0f;
    static const constexpr float nearPlane = 2.0f;
    static const constexpr float farPlane = 600.0f;
    const float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    const float nearFrustumTop = nearPlane * std::tan(fov * 0.5f * invDegreesToRadians);
    const float nearFrustumBottom = -nearFrustumTop;
    const float nearFrustumRight = nearFrustumTop * aspectRatio;
    const float nearFrustumLeft = -nearFrustumRight;
    const float farFrustumTop = farPlane * std::tan(fov * 0.5f * invDegreesToRadians);
    const float farFrustumBottom = -farFrustumTop;
    const float farFrustumRight = farFrustumTop * aspectRatio;
    const float farFrustumLeft = -farFrustumRight;
    const float getPosX();
    const float getPosY();
    const float getPosZ();
    const float getYaw();
    const float getPitch();
    const float getViewX();
    const float getViewY();
    const float getViewZ();
    const float getRoll();

private:
    float posX = 0.0f;
    float posY = 0.0f;
    float posZ = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float viewX = 0.0f;
    float viewY = 0.0f;
    float viewZ = 0.0f;
    static const constexpr float cameraSpeed = 18.0f;
    const float distance = 4.0f;
    const float maxHeight = 4.0f;
    const float factor(float &deltaTime);
    void setYaw(float &yaw);
    void setPitch(float &pitch);
};

#endif // CAMERA_H
