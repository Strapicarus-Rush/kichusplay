// camera.h

#ifndef CAMERA_H
#define CAMERA_H

#include "character.h"

class Camera {
public:
    Camera(Character* target);

    void update();

private:
    Character* target; // Pointer to the character the camera should follow
    // Define any other member variables needed for camera positioning, orientation, etc.
};

#endif // CAMERA_H
