
#include <GL/glew.h>
#include <iostream>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <cmath>
#include <thread>
#include <filesystem>
#include "playlist.h"
#include <chrono>
#include <algorithm> // For std::shuffle
#include <random>    // For std::default_random_engine
#include <numeric>    // For std::iota


const char *TITLE = "KichusPlay Game GOTY Delux Edition | By Strapicarus";
float GROUND_Y = 0.0f;
const float GRAVITY = 9.8f; // Acceleration due to gravity (m/s^2)
// Define variables to track the state of each relevant key
bool keyWPressed = false;
bool keySPressed = false;
bool keyAPressed = false;
bool keyDPressed = false;
bool keyQPressed = false;
bool keyEPressed = false;
bool keySpacePressed = false;

// Camera parameters
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;
float cameraPosZ = 0.0f;
float cameraYaw = 0.0f;   // Yaw angle (horizontal)
float cameraPitch = 0.0f; // Pitch angle (vertical)

bool debug = true;
bool initializing = true;

float textWidth = 1.0f;
float textHeight = 1.0f;
int WINDOW_WIDTH = 1920;
int WINDOW_HEIGHT = 1080;

const int SHADOW_WIDTH = 1024;
const int SHADOW_HEIGHT = 1024;

GLuint depthMapFBO;
GLuint shadowShaderProgram;
GLint lightSpaceMatrixLocation;
GLfloat lightSpaceMatrix[16];
GLuint depthMap;

SDL_GLContext context;
SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;
SDL_Texture *textTexture;
SDL_Color textColor = {255, 255, 255, 255};
SDL_Event event;

const int TERRAIN_WIDTH = 500;
const int TERRAIN_HEIGHT = 500;

float terrainData[TERRAIN_WIDTH][TERRAIN_HEIGHT];

Playlist playlist(debug);

class Timer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_LastTime;

public:
    Timer()
    {
        m_LastTime = std::chrono::high_resolution_clock::now();
    }

    float GetDeltaTime()
    {
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(now - m_LastTime).count();
        m_LastTime = now;
        return deltaTime;
    }
};

void renderSceneFromLight();
void renderSceneWithShadows();

bool isMouseCaptured = false;

class PhysObject
{
public:
    virtual void updateVerticalPosition(float deltaTime, float gravity) = 0; // Pure virtual function
};

class Player : public PhysObject
{
public:
    float posX;
    float posY;
    float lastPosY = 0.0f;
    float posZ;
    float velocityX;
    float velocityY;
    float velocityZ;
    float rotationY;
    float mass;        // Optional: mass of the object
    bool onAir = true; // Flag to track if the player is jumping

    // Constructor
    Player(float x, float y, float z) : posX(x), posY(y), posZ(z), velocityY(0.0f), rotationY(0.0f), velocityZ(0.0f), mass(1.0f) {}

    // Function to update the vertical position of the playe
    void updateVerticalPosition(float deltaTime, float gravity) override
    {

        // Perform ray casting to find the distance to the ground
        float distanceToGround = rayCastGround(posX, posY, posZ);

        // Update vertical position
        if (distanceToGround > 0.1)
        {
            onAir = true;
        }
        if (onAir)
        {
            // Apply acceleration due to gravity
            velocityY -= gravity * deltaTime;
            posY += velocityY * deltaTime;
            if (distanceToGround <= 0.1f)
            {
                onAir = false;
                velocityY = 0.0f;
            }
            std::cout << "onAir: " << onAir << std::endl;
            if (debug > 0)
            {
                std::cout << "Distance to ground: " << distanceToGround << std::endl;
            }
        }

    }

    // Function to perform ray casting to find the distance to the ground
    float rayCastGround(float x, float y, float z)
    {
        // Define the direction of the ray (pointing downwards)
        float rayDirectionX = 0.0f;
        float rayDirectionY = -1.0f;
        float rayDirectionZ = 0.0f; // Points downwards along the negative z-axis

        // Define the starting position of the ray
        float rayOriginX = x;
        float rayOriginY = y;
        float rayOriginZ = z;

        // Define the step size for the ray (adjust as needed)
        float stepSize = 0.1f;

        // Define the maximum distance to cast the ray
        float maxDistance = 100.0f; // Adjust as needed

        // Perform ray casting
        for (float distance = 0.0f; distance <= maxDistance; distance += stepSize)
        {
            // Calculate the current position along the ray
            float currentX = rayOriginX + rayDirectionX * distance;
            float currentY = rayOriginY + rayDirectionY * distance;
            float currentZ = rayOriginZ + rayDirectionZ * distance;

            // Perform collision detection with the ground (e.g., a flat plane at z = 0)
            if (currentY <= 120.0f)
            {
                // Return the distance to the ground
                return distance;
            }
        }

        // If no collision is detected within the max distance, return a large value
        return maxDistance;
    }
};

class Enemy : public PhysObject
{
public:
    float posX;
    float posY;
    float posZ;
    float speed;
    float directionX;
    float directionY;
    float directionZ;
    // Add enemy-specific members and functions here
    void updateVerticalPosition(float deltaTime, float gravity) override
    {
        // Implement vertical position update for enemy
    }
};

// Projectile class derived from PhysObject
class Projectile : public PhysObject
{
public:
    float posX;
    float posY;
    float posZ;
    float speed;
    float directionX;
    float directionY;
    float directionZ;
    // Add projectile-specific members and functions here
    void updateVerticalPosition(float deltaTime, float gravity) override
    {
        // Implement vertical position update for projectile
    }
};
std::vector<Enemy> enemies;
std::vector<Projectile> projectiles;
void setupProjectionMatrix()
{
    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspectRatio = (float)WINDOW_WIDTH / WINDOW_HEIGHT;
    float fov = 75.0f;        // Field of view (degrees)
    float nearPlane = 0.05f;  // Near clipping plane
    float farPlane = 1000.0f; // Far clipping plane
    float top = nearPlane * tan(fov * 0.5f * (3.14159f / 180.0f));
    float bottom = -top;
    float right = top * aspectRatio;
    float left = -right;
    glFrustum(left, right, bottom, top, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
}

// Function to calculate the view matrix
void calculateViewMatrix()
{
    // Apply rotation
    glRotatef(cameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-cameraYaw, 0.0f, 1.0f, 0.0f);
    // Apply translation
    glTranslatef(-cameraPosX, -cameraPosY, -cameraPosZ);
}

// Function to handle keyboard and mouse input for camera movement and rotation
void handleInput(SDL_Window *window, Player &player, float deltatime)
{
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            exit(EXIT_SUCCESS);
        }
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_w) // Forward
            {
                keyWPressed = true;
            }
            if (event.key.keysym.sym == SDLK_s) // Backward
            {
                keySPressed = true;
            }
            if (event.key.keysym.sym == SDLK_a) // Left
            {
                keyAPressed = true;
            }
            if (event.key.keysym.sym == SDLK_d) // Right
            {
                keyDPressed = true;
            }
            if (event.key.keysym.sym == SDLK_q) // Rotate left
            {
                keyQPressed = true;
            }
            if (event.key.keysym.sym == SDLK_e) // Rotate right
            {
                keyEPressed = true;
            }
            if (event.key.keysym.sym == SDLK_SPACE) // Jump
            {
                keySpacePressed = true;
            }
            // Check if the ESC key is pressed
            if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            {
                isMouseCaptured = !isMouseCaptured;
                SDL_SetRelativeMouseMode(isMouseCaptured ? SDL_TRUE : SDL_FALSE);
                SDL_ShowCursor(isMouseCaptured ? SDL_DISABLE : SDL_ENABLE);
            }
        }
        if (event.type == SDL_KEYUP)
        {
            if (event.key.keysym.sym == SDLK_w)
            {
                keyWPressed = false;
            }
            if (event.key.keysym.sym == SDLK_s)
            {
                keySPressed = false;
            }
            if (event.key.keysym.sym == SDLK_a)
            {
                keyAPressed = false;
            }
            if (event.key.keysym.sym == SDLK_d)
            {
                keyDPressed = false;
            }
            if (event.key.keysym.sym == SDLK_q)
            {
                keyQPressed = false;
            }
            if (event.key.keysym.sym == SDLK_e)
            {
                keyEPressed = false;
            }
            if (event.key.keysym.sym == SDLK_SPACE)
            {
                keySpacePressed = false;
            }
        }
        if (event.type == SDL_MOUSEBUTTONDOWN && !isMouseCaptured)
        {
            isMouseCaptured = !isMouseCaptured;
            SDL_SetRelativeMouseMode(isMouseCaptured ? SDL_TRUE : SDL_FALSE);
            SDL_ShowCursor(isMouseCaptured ? SDL_DISABLE : SDL_ENABLE);
        }

        if (isMouseCaptured)
        {
            if (event.type == SDL_MOUSEMOTION)
            {
                // if (event.motion.state)
                // {
                cameraYaw -= event.motion.xrel * 0.1f;
                cameraPitch += event.motion.yrel * 0.1f;
                if (cameraPitch > 89.0f)
                    cameraPitch = 89.0f;
                if (cameraPitch < -89.0f)
                    cameraPitch = -89.0f;
                // }
            }
        }
    }
}

// Function to update the positions of enemies
void updateEnemies()
{
    for (auto &enemy : enemies)
    {
        enemy.posX += enemy.speed * enemy.directionX;
        enemy.posY += enemy.speed * enemy.directionY;
        enemy.posZ += enemy.speed * enemy.directionZ;
    }
}

// Function to update the positions of projectiles and check for collisions
void updateProjectiles()
{
    for (auto &projectile : projectiles)
    {
        projectile.posX += projectile.speed * projectile.directionX;
        projectile.posY += projectile.speed * projectile.directionY;
        projectile.posZ += projectile.speed * projectile.directionZ;

        // Check for collision with enemies
        for (auto &enemy : enemies)
        {
            float distanceX = projectile.posX - enemy.posX;
            float distanceY = projectile.posY - enemy.posY;
            float distanceZ = projectile.posZ - enemy.posZ;
            float distanceSquared = distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ;
            if (distanceSquared < 0.1f)
            {
                // Remove enemy and projectile upon collision
                // (Implementation of removing objects from vector omitted for brevity)
            }
        }
    }
}

void drawSolidSphere(float radius, int numStacks, int numSlices)
{
    for (int i = 0; i < numStacks; ++i)
    {
        float theta1 = (float)i * M_PI / numStacks;
        float theta2 = (float)(i + 1) * M_PI / numStacks;
        float y1 = radius * cos(theta1);
        float y2 = radius * cos(theta2);
        float r1 = radius * sin(theta1);
        float r2 = radius * sin(theta2);

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= numSlices; ++j)
        {
            float phi = (float)j * 2.0f * M_PI / numSlices;
            float x = cos(phi);
            float z = sin(phi);

            glNormal3f(x * r2 / radius, y2 / radius, z * r2 / radius);
            glVertex3f(x * r2, y2, z * r2);
            glNormal3f(x * r1 / radius, y1 / radius, z * r1 / radius);
            glVertex3f(x * r1, y1, z * r1);
        }
        glEnd();
    }
}

void drawSolidCube(float size)
{
    float halfSize = size / 2.0f;

    glBegin(GL_QUADS);

    // Front face
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);

    // Back face
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(-halfSize, halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, -halfSize);

    // Top face
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-halfSize, halfSize, -halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);

    // Bottom face
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);
    glVertex3f(-halfSize, -halfSize, halfSize);

    // Right face
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);

    // Left face
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(-halfSize, -halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, -halfSize);

    glEnd();
}

void drawStickman(Player &player)
{
    // Apply player's rotation
    glPushMatrix();
    glTranslatef(player.posX, player.posY, player.posZ);
    glRotatef(player.rotationY, 0.0f, 1.0f, 0.0f);

    // Draw head (hair)
    glPushMatrix();
    glTranslatef(0.0f, 2.7f, 0.0f); // Adjust position to separate hair from skin
    glColor3f(0.1f, 0.1f, 0.1f);    // Almost black color for hair
    drawSolidSphere(0.2f, 20, 20);  // Adjust sphere radius as needed
    glPopMatrix();

    // Draw head (skin)
    glPushMatrix();
    glTranslatef(0.0f, 2.5f, 0.0f);
    glColor3f(1.0f, 0.8f, 0.6f);   // Skin color
    drawSolidSphere(0.2f, 20, 20); // Adjust sphere radius as needed
    glPopMatrix();

    // Draw body
    glPushMatrix();
    glTranslatef(0.0f, 2.0f, 0.0f);
    glColor3f(0.6f, 0.8f, 0.6f); // Light green color for body
    drawSolidCube(0.4f);         // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw arms
    glPushMatrix();
    glTranslatef(-0.3f, 2.3f, 0.0f);
    glColor3f(1.0f, 0.8f, 0.6f); // Skin color for arms
    glScalef(0.8f, 0.2f, 0.2f);  // Scale to adjust arm size
    drawSolidCube(1.0f);         // Adjust cube dimensions as needed
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.3f, 2.3f, 0.0f);
    glColor3f(1.0f, 0.8f, 0.6f); // Skin color for arms
    glScalef(0.8f, 0.2f, 0.2f);  // Scale to adjust arm size
    drawSolidCube(1.0f);         // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw hips
    glPushMatrix();
    glTranslatef(0.0f, 1.6f, 0.0f);
    glColor3f(0.0f, 0.5f, 0.75f); // Dark cyan color for hips
    glScalef(0.5f, 0.4f, 0.2f);   // Scale to adjust hip size
    drawSolidCube(1.0f);          // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw legs
    glPushMatrix();
    glTranslatef(-0.1f, 1.0f, 0.0f);
    glColor3f(0.0f, 0.5f, 0.75f); // Dark cyan color for legs
    glScalef(0.4f, 0.8f, 0.2f);   // Scale to adjust leg size
    drawSolidCube(1.0f);          // Adjust cube dimensions as needed
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f, 1.0f, 0.0f);
    glColor3f(0.0f, 0.5f, 0.75f); // Dark cyan color for legs
    glScalef(0.4f, 0.8f, 0.2f);   // Scale to adjust leg size
    drawSolidCube(1.0f);          // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw shoes
    glPushMatrix();
    glTranslatef(-0.1f, 0.5f, 0.0f);
    glColor3f(1.0f, 1.0f, 1.0f); // White color for shoes
    glScalef(0.4f, 0.2f, 0.2f);  // Scale to adjust shoe size
    drawSolidCube(1.0f);         // Adjust cube dimensions as needed
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f, 0.5f, 0.0f);
    glColor3f(1.0f, 1.0f, 1.0f); // White color for shoes
    glScalef(0.4f, 0.2f, 0.2f);  // Scale to adjust shoe size
    drawSolidCube(1.0f);         // Adjust cube dimensions as needed
    glPopMatrix();

    // Restore the previous transformation state
    glPopMatrix();
}

void renderAxis()
{
    // Render axes (x: red, y: green, z: blue)
    glBegin(GL_LINES);

    // X-axis (red)
    glColor3f(1.0f, 0.0f, 1.0f);
    glVertex3f(-10.0f, 0.0f, 1.0f);
    glVertex3f(10.0f, 0.0f, 1.0f);
    // Arrowhead for X-axis
    glVertex3f(10.0f, 0.0f, 1.0f);
    glVertex3f(9.0f, 1.0f, 1.0f);
    glVertex3f(10.0f, 0.0f, 1.0f);
    glVertex3f(9.0f, -1.0f, 1.0f);

    // Y-axis (green)
    glColor3f(0.0f, 1.0f, 1.0f);
    glVertex3f(0.0f, -10.0f, 1.0f);
    glVertex3f(0.0f, 10.0f, 1.0f);
    // Arrowhead for Y-axis
    glVertex3f(0.0f, 10.0f, 1.0f);
    glVertex3f(1.0f, 9.0f, 1.0f);
    glVertex3f(0.0f, 10.0f, 1.0f);
    glVertex3f(-1.0f, 9.0f, 1.0f);

    // Z-axis (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, -10.0f);
    glVertex3f(0.0f, 0.0f, 10.0f);
    // Arrowhead for Z-axis
    glVertex3f(0.0f, 0.0f, 10.0f);
    glVertex3f(0.0f, 1.0f, 9.0f);
    glVertex3f(0.0f, 0.0f, 10.0f);
    glVertex3f(0.0f, -1.0f, 9.0f);

    glEnd();
}

// Function to update camera position following the player with acceleration and deceleration
void updateCameraPosition(Player &player, float deltatime)
{
    float cameraSpeed = 14.0f;
    float distance = 1.2f;
    float height = 2.0f;

    // Convert cameraYaw from degrees to radians
    float yawRad = cameraYaw * (M_PI / 180.0f);

    // Calculate the direction from the camera to the player
    float directionX = player.posX - cameraPosX;
    float directionY = player.posY - cameraPosY;
    float directionZ = player.posZ - cameraPosZ;

    // Normalize the direction
    float length = sqrt(directionX * directionX + directionY * directionY + directionZ * directionZ);
    directionX /= length;
    directionY /= length;
    directionZ /= length;

    // Calculate the desired position of the camera behind the player
    float targetX = player.posX - directionX + distance * std::sin(yawRad);
    float targetY = player.posY - directionY + height;
    float targetZ = player.posZ - directionZ + distance * std::cos(yawRad);

    // Calculate the velocity of the camera towards the desired position
    float factor = 1.0f - std::exp(-cameraSpeed * deltatime);
    float velocityX = (targetX - cameraPosX) * factor;
    float velocityY = (targetY - cameraPosY) * factor;
    float velocityZ = (targetZ - cameraPosZ) * factor;

    // Update the camera position with the calculated velocity
    cameraPosX += velocityX;
    cameraPosY += velocityY;
    cameraPosZ += velocityZ;

    // Update player rotation to match camera rotation
    player.rotationY = -atan2f(directionX, -directionZ) * (180.0f / M_PI);
}

void updatePhysics(std::vector<PhysObject *> &physObjects, float gravity, float deltatime)
{
    for (auto &obj : physObjects)
    {
        obj->updateVerticalPosition(deltatime, gravity);
    }
}

void checkGLError()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error: " << error << std::endl;
    }
}

void checkKeyStatus(Player &player, float deltatime)
{
    // Calculate movement direction based on camera's yaw angle
    float directionX = cos(cameraYaw * (M_PI / 180.0f));
    float directionZ = -sin(cameraYaw * (M_PI / 180.0f));
    float velocity = 5.0f;
    float jump = velocity * 2.0f;
    float movementSpeed = velocity * deltatime; // Adjust movement speed as needed

    if (keyWPressed)
    {
        player.posX += directionZ * movementSpeed;
        player.posZ -= directionX * movementSpeed;
    }
    if (keySPressed)
    {
        player.posX -= directionZ * movementSpeed;
        player.posZ += directionX * movementSpeed;
    }
    if (keyAPressed)
    {
        player.posX -= directionX * movementSpeed;
        player.posZ -= directionZ * movementSpeed;
    }
    if (keyDPressed)
    {
        player.posX += directionX * movementSpeed;
        player.posZ += directionZ * movementSpeed;
    }
    if (keyQPressed)
    {
        player.rotationY -= 5.0f;
    }
    if (keyEPressed)
    {
        player.rotationY += 5.0f;
    }
    if (keySpacePressed && !player.onAir)
    {
        player.onAir = true;
        player.velocityY += jump;
    }
}

// void precalculateTerrain(Player &player) {
//     for (int x = 0; x < TERRAIN_WIDTH; ++x) {
//         for (int z = 0; z < TERRAIN_HEIGHT; ++z) {
//             // Calcular la altura en la posición actual
//             float height = getHeight(player.posX - WINDOW_WIDTH / 2 + x, player.posZ - WINDOW_HEIGHT / 2 + z);
//             terrainData[x][z] = height;
//         }
//     }
// }

// // Define regions and their properties
// struct TerrainRegion
// {
//     float minHeight;
//     float maxHeight;
//     float red, green, blue; // Color components
// };
// std::vector<TerrainRegion> regions = {
//     {0.1f, 0.8f, 0.0f, 0.0f, 1.0f},   // Blue - Water
//     {0.9f, 1.2f, 0.4f, 0.2f, 0.0f},  // Brown - Dirt
//     {1.3f, 15.0f, 1.0f, 1.0f, 0.0f}, // Yellowish - Sand
//     {15.1f, 30.0f, 0.0f, 1.0f, 0.0f},  // Green - Grassland
//     {30.1f, 50.0f, 0.0f, 0.5f, 0.0f},  // Dark Green - Valley
//     {50.1f, 75.0f, 1.0f, 1.0f, 0.0f},  // Yellowish - Sand (Coline)
//     {96.1f, 120.0f, 0.6f, 0.6f, 0.6f}, // Gray - Mountain
//     {120.1f, 150.0f, 1.0f, 1.0f, 1.0f} // White - Peak Snow
// };
// // Terrain colors
// const GLfloat dirtColor[3] = {0.5f, 0.35f, 0.05f};
// const GLfloat grasslandColor[3] = {0.0f, 1.0f, 0.0f};
// const GLfloat valleyColor[3] = {0.0f, 0.5f, 0.0f};
// const GLfloat colineColor[3] = {0.9f, 0.85f, 0.6f};
// const GLfloat highlandsColor[3] = {0.5f, 0.35f, 0.1f};
// const GLfloat mountainColor[3] = {0.6f, 0.6f, 0.6f};
// const GLfloat peakSnowColor[3] = {1.0f, 1.0f, 1.0f};
// const GLfloat sandColor[3] = {0.9f, 0.85f, 0.6f};
// const GLfloat waterColor[3] = {0.0f, 0.0f, 0.8f};

// // Get the color for a given height value based on terrain regions
// void getColor(float height, float &red, float &green, float &blue)
// {
//     for (const auto &region : regions)
//     {
//         if (height >= region.minHeight && height <= region.maxHeight)
//         {
//             red = region.red;
//             green = region.green;
//             blue = region.blue;
//             return;
//         }
//     }
// }
// // Function to generate height based on player position
// GLfloat getHeight(GLfloat x, GLfloat z)
// {
//     // Define regions
//     const GLfloat region1Start = 0.0f;
//     const GLfloat region2Start = 500.0f;
//     const GLfloat region3Start = 1000.0f;

//     // Define frequencies for sine functions
//     const GLfloat freq1 = 1.0f;
//     const GLfloat freq2 = 0.5f;
//     const GLfloat freq3 = 0.2f;

//     // Generate height based on regions
//     if (z <= region1Start)
//     {
//         return std::sin(freq1 * x) * std::cos(freq1 * z);
//     }
//     else if (z <= region2Start)
//     {
//         return std::sin(freq2 * x) * std::cos(freq2 * z);
//     }
//     else
//     {
//         return std::sin(freq3 * x) * std::cos(freq3 * z);
//     }
// }



// void renderTerrain(Player &player)
// {
//     // Rango ajustado para evitar problemas de rendimiento
//     float startX = player.posX - 120;
//     float endX = player.posX + 120;
//     float startZ = player.posZ - 120;
//     float endZ = player.posZ + 120;

//     for (GLfloat x = startX; x < endX; x += 0.4f)
//     {
//         for (GLfloat z = startZ; z < endZ; z += 0.4f)
//         {
//             // Get height at current position
//             GLfloat height = getHeight(x, z);

//             // Determine color based on height
//             const GLfloat *color;
//             if (height < 0.01f)
//             {
//                 color = waterColor;
//             }
//             else if (height < 0.1f)
//             {
//                 color = sandColor;
//             }
//             else if (height < 1.0f)
//             {
//                 color = dirtColor;
//             }
//             else if (height < 1.3f)
//             {
//                 color = dirtColor;
//             }

//             // Render terrain quad
//             glBegin(GL_QUADS);
//             glColor3fv(color);
//             glVertex3f(x, height, z);
//             glVertex3f(x + 0.4f, height, z);
//             glVertex3f(x + 0.4f, height, z + 0.4f);
//             glVertex3f(x, height, z + 0.4f);
//             glEnd();
//         }
//     }
// }
// class PerlinNoise {
// public:
//     PerlinNoise() {
//         std::iota(p, p + 256, 0); // Fill p with values from 0 to 255
//         std::default_random_engine engine(std::random_device{}());
//         std::shuffle(p, p + 256, engine); // Shuffle using std::shuffle
//         for (int i = 0; i < 256; ++i) {
//             p[256 + i] = p[i];
//         }
//     }

//     float noise(float x, float y, float z) const {
//         int X = static_cast<int>(floor(x)) & 255;
//         int Y = static_cast<int>(floor(y)) & 255;
//         int Z = static_cast<int>(floor(z)) & 255;

//         x -= floor(x);
//         y -= floor(y);
//         z -= floor(z);

//         float u = fade(x);
//         float v = fade(y);
//         float w = fade(z);

//         int A = p[X] + Y;
//         int AA = p[A] + Z;
//         int AB = p[A + 1] + Z;
//         int B = p[X + 1] + Y;
//         int BA = p[B] + Z;
//         int BB = p[B + 1] + Z;

//         return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
//                                       grad(p[BA], x - 1, y, z)),
//                               lerp(u, grad(p[AB], x, y - 1, z),
//                                       grad(p[BB], x - 1, y - 1, z))),
//                        lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
//                                       grad(p[BA + 1], x - 1, y, z - 1)),
//                               lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
//                                       grad(p[BB + 1], x - 1, y - 1, z - 1))));
//     }

// private:
//     int p[512];

//     float fade(float t) const {
//         return t * t * t * (t * (t * 6 - 15) + 10);
//     }

//     float lerp(float t, float a, float b) const {
//         return a + t * (b - a);
//     }

//     float grad(int hash, float x, float y, float z) const {
//         int h = hash & 15;
//         float u = h < 8 ? x : y;
//         float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
//         return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
//     }
// };

// // Define regions and their properties
// struct TerrainRegion {
//     float minHeight;
//     float maxHeight;
//     float red, green, blue; // Color components
// };

// std::vector<TerrainRegion> regions = {
//     {0.0f, 0.8f, 0.0f, 0.0f, 1.0f},    // Blue - Water
//     {0.9f, 1.2f, 0.4f, 0.2f, 0.0f},    // Brown - Dirt
//     {1.3f, 15.0f, 1.0f, 1.0f, 0.0f},   // Yellowish - Sand
//     {15.1f, 30.0f, 0.0f, 1.0f, 0.0f},  // Green - Grassland
//     {30.1f, 50.0f, 0.0f, 0.5f, 0.0f},  // Dark Green - Valley
//     {50.1f, 75.0f, 1.0f, 1.0f, 0.0f},  // Yellowish - Sand (Coline)
//     {75.1f, 96.0f, 0.5f, 0.35f, 0.1f}, // Light Brown - Highlands
//     {96.1f, 120.0f, 0.6f, 0.6f, 0.6f}, // Gray - Mountain
//     {120.1f, 150.0f, 1.0f, 1.0f, 1.0f} // White - Peak Snow
// };

// // Get the color for a given height value based on terrain regions
// void getColor(float height, float &red, float &green, float &blue) {
//     for (const auto &region : regions) {
//         if (height >= region.minHeight && height <= region.maxHeight) {
//             red = region.red;
//             green = region.green;
//             blue = region.blue;
//             return;
//         }
//     }
// }

// // Generate height using Perlin noise
// GLfloat getHeight(GLfloat x, GLfloat z, const PerlinNoise &noise) {
//     float scale = 0.1f;
//     float height = noise.noise(x * scale, z * scale, 0.0f) * 100.0f;
//     return height;
// }

// // Calculate normal for a terrain point
// void calculateNormal(GLfloat x, GLfloat z, GLfloat &nx, GLfloat &ny, GLfloat &nz, const PerlinNoise &noise) {
//     GLfloat hL = getHeight(x - 1.0f, z, noise);
//     GLfloat hR = getHeight(x + 1.0f, z, noise);
//     GLfloat hD = getHeight(x, z - 1.0f, noise);
//     GLfloat hU = getHeight(x, z + 1.0f, noise);
//     nx = hL - hR;
//     ny = 2.0f;
//     nz = hD - hU;
//     GLfloat length = sqrt(nx * nx + ny * ny + nz * nz);
//     nx /= length;
//     ny /= length;
//     nz /= length;
// }

// // Render the terrain
// void renderTerrain(Player &player) {
//     PerlinNoise noise;

//     float startX = player.posX - 120;
//     float endX = player.posX + 120;
//     float startZ = player.posZ - 120;
//     float endZ = player.posZ + 120;

//     for (GLfloat x = startX; x < endX; x += 0.4f) {
//         for (GLfloat z = startZ; z < endZ; z += 0.4f) {
//             // Get height at current position
//             GLfloat height = getHeight(x, z, noise);

//             // Determine color based on height
//             float red, green, blue;
//             getColor(height, red, green, blue);

//             // Calculate normals for lighting
//             GLfloat nx, ny, nz;
//             calculateNormal(x, z, nx, ny, nz, noise);

//             // Render terrain quad
//             glBegin(GL_QUADS);
//             glColor3f(red, green, blue);
//             glNormal3f(nx, ny, nz);
//             glVertex3f(x, height, z);
//             GLfloat h1 = getHeight(x + 0.4f, z, noise);
//             GLfloat h2 = getHeight(x + 0.4f, z + 0.4f, noise);
//             GLfloat h3 = getHeight(x, z + 0.4f, noise);
//             glVertex3f(x + 0.4f, h1, z);
//             glVertex3f(x + 0.4f, h2, z + 0.4f);
//             glVertex3f(x, h3, z + 0.4f);
//             glEnd();
//         }
//     }
// }

class PerlinNoise {
public:
    PerlinNoise() {
        std::iota(p, p + 256, 0); // Fill p with values from 0 to 255
        std::default_random_engine engine(std::random_device{}());
        std::shuffle(p, p + 256, engine); // Shuffle using std::shuffle
        for (int i = 0; i < 256; ++i) {
            p[256 + i] = p[i];
        }
    }

    float noise(float x, float y, float z) const {
        int X = static_cast<int>(floor(x)) & 255;
        int Y = static_cast<int>(floor(y)) & 255;
        int Z = static_cast<int>(floor(z)) & 255;

        x -= floor(x);
        y -= floor(y);
        z -= floor(z);

        float u = fade(x);
        float v = fade(y);
        float w = fade(z);

        int A = p[X] + Y;
        int AA = p[A] + Z;
        int AB = p[A + 1] + Z;
        int B = p[X + 1] + Y;
        int BA = p[B] + Z;
        int BB = p[B + 1] + Z;

        return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                      grad(p[BA], x - 1, y, z)),
                              lerp(u, grad(p[AB], x, y - 1, z),
                                      grad(p[BB], x - 1, y - 1, z))),
                       lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
                                      grad(p[BA + 1], x - 1, y, z - 1)),
                              lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                      grad(p[BB + 1], x - 1, y - 1, z - 1))));
    }

private:
    int p[512];

    float fade(float t) const {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    float lerp(float t, float a, float b) const {
        return a + t * (b - a);
    }

    float grad(int hash, float x, float y, float z) const {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};

// Define regions and their properties
struct TerrainRegion {
    float minHeight;
    float maxHeight;
    float red, green, blue; // Color components
};

std::vector<TerrainRegion> regions = {
    {0.0f, 0.8f, 0.0f, 0.0f, 1.0f},    // Blue - Water
    {0.9f, 1.2f, 0.4f, 0.2f, 0.0f},    // Brown - Dirt
    {1.3f, 5.0f, 1.0f, 1.0f, 0.0f},   // Yellowish - Sand
    {5.1f, 10.0f, 0.0f, 1.0f, 0.0f},  // Green - Grassland
    {10.1f, 20.0f, 0.0f, 0.5f, 0.0f},  // Dark Green - Valley
    {20.1f, 25.0f, 1.0f, 1.0f, 0.0f},  // Yellowish - Sand (Coline)
    {25.1f, 46.0f, 0.5f, 0.35f, 0.1f}, // Light Brown - Highlands
    {46.1f, 70.0f, 0.6f, 0.6f, 0.6f}, // Gray - Mountain
    {70.1f, 120.0f, 1.0f, 1.0f, 1.0f} // White - Peak Snow
};

// Get the color for a given height value based on terrain regions
void getColor(float height, float &red, float &green, float &blue) {
    for (const auto &region : regions) {
        if (height >= region.minHeight && height <= region.maxHeight) {
            red = region.red;
            green = region.green;
            blue = region.blue;
            return;
        }
    }
}

// Create a heightmap using Perlin noise
std::vector<std::vector<GLfloat>> createHeightmap(int width, int height, const PerlinNoise &noise) {
    std::vector<std::vector<GLfloat>> heightmap(width, std::vector<GLfloat>(height));
    for (int x = 0; x < width; ++x) {
        for (int z = 0; z < height; ++z) {
            float scale = 0.009f;
            heightmap[x][z] = noise.noise(x * scale, z * scale, 0.0f) * 100.0f;
        }
    }
    return heightmap;
}

// Calculate normal for a terrain point using the heightmap
void calculateNormal(int x, int z, GLfloat &nx, GLfloat &ny, GLfloat &nz, const std::vector<std::vector<GLfloat>> &heightmap) {
    int width = heightmap.size();
    int height = heightmap[0].size();

    GLfloat hL = (x > 0) ? heightmap[x - 1][z] : heightmap[x][z];
    GLfloat hR = (x < width - 1) ? heightmap[x + 1][z] : heightmap[x][z];
    GLfloat hD = (z > 0) ? heightmap[x][z - 1] : heightmap[x][z];
    GLfloat hU = (z < height - 1) ? heightmap[x][z + 1] : heightmap[x][z];

    nx = hL - hR;
    ny = 2.0f;
    nz = hD - hU;

    GLfloat length = sqrt(nx * nx + ny * ny + nz * nz);
    nx /= length;
    ny /= length;
    nz /= length;
}

void renderTerrain(Player &player, const std::vector<std::vector<GLfloat>> &heightmap) {
    int width = heightmap.size();
    int height = heightmap[0].size();

    // Adjust render range based on player position
    float startX = std::max(player.posX - 140, 0.0f);
    float endX = std::min(player.posX + 140, static_cast<float>(width));
    float startZ = std::max(player.posZ - 140, 0.0f);
    float endZ = std::min(player.posZ + 140, static_cast<float>(height));

    for (float x = startX; x < endX; x += 1.0f) {
        for (float z = startZ; z < endZ; z += 1.0f) {
            // Get height at current position from the heightmap
            int xIndex = static_cast<int>(x);
            int zIndex = static_cast<int>(z);
            GLfloat height = heightmap[xIndex][zIndex];

            // Determine color based on height
            float red, green, blue;
            getColor(height, red, green, blue);

            // Calculate normals for lighting
            GLfloat nx, ny, nz;
            calculateNormal(xIndex, zIndex, nx, ny, nz, heightmap);

            // Render terrain quad
            glBegin(GL_QUADS);
            glColor3f(red, green, blue);
            glNormal3f(nx, ny, nz);
            glVertex3f(x, height, z);
            glVertex3f(x + 1.0f, heightmap[xIndex + 1][zIndex], z);
            glVertex3f(x + 1.0f, heightmap[xIndex + 1][zIndex + 1], z + 1.0f);
            glVertex3f(x, heightmap[xIndex][zIndex + 1], z + 1.0f);
            glEnd();
        }
    }
}

// void renderTerrain(Player &player, const PerlinNoise &noise) {
//     // Define terrain parameters
//     float terrainSize = 300.0f; // Size of the terrain
//     float step = 1.0f; // Step size between vertices
//     int detailLevel = 2; // Level of detail for terrain

//     // Determine the range of terrain to render around the player
//     float startX = player.posX - terrainSize / 2.0f;
//     float endX = player.posX + terrainSize / 2.0f;
//     float startZ = player.posZ - terrainSize / 2.0f;
//     float endZ = player.posZ + terrainSize / 2.0f;

//     // Render terrain in the specified range
//     for (float x = startX; x < endX; x += step * detailLevel) {
//         for (float z = startZ; z < endZ; z += step * detailLevel) {
//             // Calculate height using Perlin noise
//             float height = noise.noise(x, 0, z) * 100.0f;

//             // Determine color based on height
//             float red, green, blue;
//             getColor(height, red, green, blue);

//             // Render terrain quad
//             glBegin(GL_QUADS);
//             glColor3f(red, green, blue);
//             glVertex3f(x, height, z);
//             glVertex3f(x + step * detailLevel, noise.noise(x + step *detailLevel, 0, z),
//                        z);
//             glVertex3f(x + step * detailLevel, noise.noise(x + step * detailLevel, 0, z + step * detailLevel),
//                        z + step * detailLevel);
//             glVertex3f(x, noise.noise(x, 0, z + step * detailLevel), z + step * detailLevel);
//             glEnd();
//         }
//     }
// }
// Render the terrain using the precomputed heightmap
// void renderTerrain(Player &player, const std::vector<std::vector<GLfloat>> &heightmap) {
//     int width = heightmap.size();
//     int height = heightmap[0].size();

//     float startX = player.posX - 120;
//     float endX = player.posX + 120;
//     float startZ = player.posZ - 120;
//     float endZ = player.posZ + 120;

//     for (int x = startX; x < endX; x += 1) {
//         for (int z = startZ; z < endZ; z += 1) {
//             if (x >= 0 && x < width - 1 && z >= 0 && z < height - 1) {
//                 GLfloat height = heightmap[x][z];

//                 // Determine color based on height
//                 float red, green, blue;
//                 getColor(height, red, green, blue);

//                 // Calculate normals for lighting
//                 GLfloat nx, ny, nz;
//                 calculateNormal(x, z, nx, ny, nz, heightmap);

//                 // Render terrain quad
//                 glBegin(GL_QUADS);
//                 glColor3f(red, green, blue);
//                 glNormal3f(nx, ny, nz);
//                 glVertex3f(x, height, z);
//                 glVertex3f(x + 1, heightmap[x + 1][z], z);
//                 glVertex3f(x + 1, heightmap[x + 1][z + 1], z + 1);
//                 glVertex3f(x, heightmap[x][z + 1], z + 1);
//                 glEnd();
//             }
//         }
//     }
// }

SDL_Texture *renderText(SDL_Renderer *renderer, TTF_Font *font, const std::string &text, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface)
    {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

TTF_Font *loadFont(const std::string &fontPath, int fontSize)
{
    TTF_Font *font = TTF_OpenFont(fontPath.c_str(), fontSize);
    if (!font)
    {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }
    return font;
}

void renderScene(SDL_Window *window, Player &player, float deltaTime, const std::vector<std::vector<GLfloat>> &heightmap)
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    calculateViewMatrix();

    // Render the axis
    renderAxis();

    // Render terrain
    renderTerrain(player, heightmap);

    // Render player
    glPushMatrix();
    drawStickman(player);
    glPopMatrix();

    // Render the text to a texture if it hasn't been rendered yet
    // if (!textTexture)
    // {
    //     textTexture = renderText(renderer, font, "strapicarus god", textColor);
    //     if (textTexture)
    //     {
    //         SDL_GL_BindTexture(textTexture, NULL, NULL);

    //         // Set up orthogonal projection for 2D rendering
    //         glMatrixMode(GL_PROJECTION);
    //         glLoadIdentity();
    //         glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    //         glMatrixMode(GL_MODELVIEW);
    //         glLoadIdentity();

    //         // Draw the textured quad
    //         glBegin(GL_QUADS);
    //         glTexCoord2f(0, 0);
    //         glVertex2f(1.0f, 1.0f);
    //         glTexCoord2f(1, 0);
    //         glVertex2f(1.0f + textWidth, 1.0f);
    //         glTexCoord2f(1, 1);
    //         glVertex2f(1.0f + textWidth, 1.0f + textHeight);
    //         glTexCoord2f(0, 1);
    //         glVertex2f(1.0f, 1.0f + textHeight);
    //         glEnd();

    //         SDL_GL_UnbindTexture(textTexture);
    //     }
    // }

    // Swap buffers
    SDL_GL_SwapWindow(window);
}

int initializeO_S(int argc, char *argv[])
{
    if (argc > 1 && argv[1] == "-debug")
    {
        debug = true;
        std::cout << "Debugging..." << std::endl;
    }
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }
    window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return EXIT_FAILURE;
    }
    context = SDL_GL_CreateContext(window);
    if (!context)
    {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    if (TTF_Init() == -1)
    {
        std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    font = TTF_OpenFont("assets/fonts/fonts-japanese-mincho.ttf", 24);
    if (!font)
    {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    // createHouse(houseVAO, houseVBO, houseEBO);
    return 0;
}

int main(int argc, char *argv[])
{
    initializeO_S(argc, argv);

    // Set up OpenGL
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Initialize enemies
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;

    // Create a player instance
    Player player(100.0f, 20.0f, 100.0f);

    // precalculateTerrain(player);
    PerlinNoise noise;
    int terrainWidth = 1000;
    int terrainHeight = 1000;
    auto heightmap = createHeightmap(terrainWidth, terrainHeight, noise);


    std::thread playlistThread([&]()
                               { playlist.addSongsFromDirectory("assets/media/music"); });

    Timer timer;
    bool quit = false;

    while (!quit)
    {

        if (event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                WINDOW_WIDTH = event.window.data1;
                WINDOW_HEIGHT = event.window.data2;
                // int newWidth = event.window.data1;
                // int newHeight = event.window.data2;
                // printf("Window resized to: %d x %d\n", newWidth, newHeight);
            }
        }
        float deltaTime = timer.GetDeltaTime();

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Handle input
        handleInput(window, player, deltaTime);
        checkKeyStatus(player, deltaTime);
        updateCameraPosition(player, deltaTime);
        // updateEnemies();
        // updateProjectiles();
        std::vector<PhysObject *> physObjects;
        physObjects.push_back(reinterpret_cast<PhysObject *>(&player));

        updatePhysics(physObjects, GRAVITY, deltaTime);
        setupProjectionMatrix();
        // Render the scene
        renderScene(window, player, deltaTime, heightmap);

        if (playlist.loading == 0 && initializing == 1)
        {
            std::cout << "Playlist Loading: " << playlist.loading << std::endl;
            playlistThread.join();
            playlist.play();
            initializing = false;
            // std::thread terrainThread([&]() // leads to segmentation fault, all render should be on render scene on maion thread
            //                           { renderTerrain(player); });
        }
        else if (playlist.loading == 0 && initializing == 0)
        {
            playlist.playNextIfFinished();
            // std::cout << "Loading: " << playlist.loading << std::endl;
        }
        // if(SDL_GetError()>0){
        //     std::cout << "SDL:GetError: " << SDL_GetError() << std::endl;
        // }
    }

    // Cleanup
    // glDeleteVertexArrays(1, &houseVAO);
    // glDeleteBuffers(1, &houseVBO);
    // glDeleteBuffers(1, &houseEBO);
    TTF_Quit();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
