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

bool debug = false;
bool initializing = true;

float textWidth = 1.0f;
float textHeight = 1.0f;

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

// Load playlist asynchronously
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

// position += (target-position) * (1 -exp(-speed*dt))
// lerp: position = lerp(position, target, 1 - exp(- speed * dt))
// position = lerp(target, position, exp(- speed * dt))
struct Vector3
{
    float x;
    float y;
    float z;
};

Vector3 interpolate(const Vector3 &position, const Vector3 &target, float speed, float dt)
{
    float factor = 1.0f - std::exp(-speed * dt);
    Vector3 result;
    result.x = position.x + (target.x - position.x) * factor;
    result.y = position.y + (target.y - position.y) * factor;
    result.z = position.z + (target.z - position.z) * factor;
    return result;
}

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
    float posZ;
    float velocityX;
    float velocityY;
    float velocityZ;
    float rotationY;
    float mass;         // Optional: mass of the object
    bool onAir = false; // Flag to track if the player is jumping

    // Constructor
    Player(float x, float y, float z) : posX(x), posY(y), posZ(z), velocityY(0.0f), rotationY(0.0f), velocityZ(0.0f), mass(1.0f) {}

    // Function to update the vertical position of the playe
    void updateVerticalPosition(float deltaTime, float gravity) override
    {
        // Apply acceleration due to gravity
        velocityY -= gravity * deltaTime;

        // Perform ray casting to find the distance to the ground
        float distanceToGround = rayCastGround(posX, posY, posZ);

        // Update vertical position
        if (onAir)
        {
            posY += velocityY * deltaTime;
        }

        if (posY <= distanceToGround) // If player's Y position is less than or equal to 0
        {
            posY = 0.0f;      // Ensure the player is at ground level
            onAir = false;    // Reset jumping flag
            velocityY = 0.0f; // Reset vertical velocity
        }
    }

    // Function to perform ray casting to find the distance to the ground
    float rayCastGround(float x, float y, float z)
    {
        // Define the direction of the ray (pointing downwards)
        float rayDirectionX = 0.0f;
        float rayDirectionY = 0.0f;
        float rayDirectionZ = -1.0f; // Points downwards along the negative z-axis

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
            if (currentZ <= 0.0f)
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

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const char *TITLE = "KichusPlay Game GOTY Delux Edition | By Strapicarus";

// float DELTATIME = 0.04f; // Adjust as needed

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

// Player parameters
// float playerPosX = 0.0f;
// float playerPosY = 0.0f;
// float playerPosZ = 2.0f;
float PLAYER_SIZE = 2.0f;

// Camera parameters
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;
float cameraPosZ = 0.0f;
float cameraYaw = 0.0f;  // Yaw angle (horizontal)
float cameraPitch = 0.0f; // Pitch angle (vertical)

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
    SDL_Event event;
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
                cameraYaw += event.motion.xrel * 0.1f;
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

void drawStickman(float posX, float posY, float posZ)
{
    // Set drawing color
    glColor3f(1.0f, 0.0f, 0.0f); // Red color

    // Draw head
    glPushMatrix();
    glTranslatef(posX, posY + 1.8f, posZ);
    drawSolidSphere(0.2f, 20, 20); // Adjust sphere radius as needed
    glPopMatrix();

    // Draw body
    glPushMatrix();
    glTranslatef(posX, posY + 0.5f, posZ);
    drawSolidCube(0.4f); // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw arms
    glPushMatrix();
    glTranslatef(posX - 0.3f, posY + 1.0f, posZ);
    glScalef(0.8f, 0.2f, 0.2f); // Scale to adjust arm size
    drawSolidCube(1.0f);        // Adjust cube dimensions as needed
    glPopMatrix();

    glPushMatrix();
    glTranslatef(posX + 0.3f, posY + 1.0f, posZ);
    glScalef(0.8f, 0.2f, 0.2f); // Scale to adjust arm size
    drawSolidCube(1.0f);        // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw legs
    glPushMatrix();
    glTranslatef(posX - 0.1f, posY - 0.5f, posZ);
    glScalef(0.4f, 0.8f, 0.2f); // Scale to adjust leg size
    drawSolidCube(1.0f);        // Adjust cube dimensions as needed
    glPopMatrix();

    glPushMatrix();
    glTranslatef(posX + 0.1f, posY - 0.5f, posZ);
    glScalef(0.4f, 0.8f, 0.2f); // Scale to adjust leg size
    drawSolidCube(1.0f);        // Adjust cube dimensions as needed
    glPopMatrix();
}

void renderAxis()
{
    // Render axes (x: red, y: green, z: blue)
    glBegin(GL_LINES);

    // X-axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-10.0f, 0.0f, 0.0f);
    glVertex3f(10.0f, 0.0f, 0.0f);
    // Arrowhead for X-axis
    glVertex3f(10.0f, 0.0f, 0.0f);
    glVertex3f(9.0f, 1.0f, 0.0f);
    glVertex3f(10.0f, 0.0f, 0.0f);
    glVertex3f(9.0f, -1.0f, 0.0f);

    // Y-axis (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, -10.0f, 0.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    // Arrowhead for Y-axis
    glVertex3f(0.0f, 10.0f, 0.0f);
    glVertex3f(1.0f, 9.0f, 0.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    glVertex3f(-1.0f, 9.0f, 0.0f);

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
    float cameraSpeed = 0.2f;
    float distance = 1.0f;
    float height = 1.0f;

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
    float targetX = player.posX;
    float targetY = player.posY - directionY + height;
    float targetZ = player.posZ - directionZ * distance;

    // Calculate the velocity of the camera towards the desired position
    float velocityX = (targetX - cameraPosX) * cameraSpeed;
    float velocityY = (targetY - cameraPosY) * cameraSpeed;
    float velocityZ = (targetZ - cameraPosZ) * cameraSpeed;

    // Update the camera position with the calculated velocity
    cameraPosX += velocityX;
    cameraPosY += velocityY;
    cameraPosZ += velocityZ;

    // Update player rotation to match camera rotation
    player.rotationY = -atan2f(directionX, -directionZ) * (180.0f / M_PI);
}

// Function to update physics for enemies and projectiles
void updatePhysics(std::vector<PhysObject *> &physObjects, float gravity, float deltatime)
{

    // Update vertical position of each object
    for (auto &obj : physObjects)
    {
        obj->updateVerticalPosition(deltatime, gravity);
    }
}

// Shader sources
const char *depthVertexShaderSource =
    "#version 440 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 lightSpaceMatrix;\n"
    "uniform mat4 model;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);\n"
    "}\n";

const char *depthFragmentShaderSource =
    "#version 440 core\n"
    "void main()\n"
    "{\n"
    "}\n";

const char *shadowVertexShaderSource =
    "#version 440 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "out vec4 FragPosLightSpace;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 lightSpaceMatrix;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "   FragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0);\n"
    "}\n";

const char *shadowFragmentShaderSource =
    "#version 440 core\n"
    "in vec4 FragPosLightSpace;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D shadowMap;\n"
    "void main()\n"
    "{\n"
    "   vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;\n"
    "   projCoords = projCoords * 0.5 + 0.5;\n"
    "   float closestDepth = texture(shadowMap, projCoords.xy).r;\n"
    "   float currentDepth = projCoords.z;\n"
    "   float shadow = currentDepth > closestDepth ? 1.0 : 0.0;\n"
    "   FragColor = vec4(0.5, 0.5, 0.5, 1.0) * (1.0 - shadow);\n"
    "}\n";

// Function to check for OpenGL errors
void checkGLError()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error: " << error << std::endl;
    }
}

// Function to compile a shader
GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    // Check compilation status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader Compilation Error: " << infoLog << std::endl;
    }
    return shader;
}

// Function to create a shader program
GLuint createShaderProgram(const char *vertexSource, const char *fragmentSource)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    // Create program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    // Check linking status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader Program Linking Error: " << infoLog << std::endl;
    }
    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

void checkKeyStatus(Player &player, float deltatime)
{
    // Calculate movement direction based on camera's yaw angle
    float directionX = cos(player.rotationY * (M_PI / 180.0f));
    float directionZ = -sin(player.rotationY * (M_PI / 180.0f));

    // Update game state
    float movementSpeed = 0.1f; // Adjust movement speed as needed
    if (keyWPressed)
    {
        // Move forward
        // player.posZ -= movementSpeed;
        player.posX += directionX * movementSpeed;
        player.posZ += directionZ * movementSpeed;
    }
    if (keySPressed)
    {
        // Move backward
        // player.posZ += movementSpeed;
        player.posX -= directionX * movementSpeed;
        player.posZ -= directionZ * movementSpeed;
    }
    if (keyAPressed)
    {
        // Move left
        // player.posX -= movementSpeed;
        player.posX -= directionZ * movementSpeed;
        player.posZ += directionX * movementSpeed;
    }
    if (keyDPressed)
    {
        // Move right
        // player.posX += movementSpeed;
        player.posX += directionZ * movementSpeed;
        player.posZ -= directionX * movementSpeed;
    }
    if (keyQPressed)
    {
        // Rotate left
        player.rotationY -= 5.0f; // Adjust rotation angle (negative direction)
    }
    if (keyEPressed)
    {
        // Rotate right
        player.rotationY += 5.0f; // Adjust rotation angle (positive direction)
    }
    if (keySpacePressed && !player.onAir)
    {
        // jump
        // if(player.velocityY < 20.0f){
        player.velocityY += 18.0f;
        // }
        player.posY += player.velocityY * deltatime;
    }
    else
    {
        player.velocityY -= GRAVITY;
        player.posY += player.velocityY * deltatime;
    }
}

// Define regions and their properties
struct TerrainRegion
{
    float minHeight;
    float maxHeight;
    float red, green, blue; // Color components
};
std::vector<TerrainRegion> regions = {
    {0.0f, 10.0f, 0.4f, 0.2f, 0.0f},  // Brown - Dirt
    {10.1f, 50.0f, 1.0f, 1.0f, 0.0f}, // Yellowish - Sand
    {50.1f, 500.0f, 0.0f, 0.0f, 1.0f} // Blue - Water
};
// Terrain colors
const GLfloat dirtColor[3] = {0.5f, 0.35f, 0.05f};
const GLfloat sandColor[3] = {0.9f, 0.85f, 0.6f};
const GLfloat waterColor[3] = {0.0f, 0.0f, 0.8f};
// Generate height value for a given x position using sine functions
float generateHeight(float x, float z)
{
    float height = std::sin(x * 0.1f) * std::cos(z * 0.1f) * 20.0f; // Sine function with smooth variation
    return height;

    // return std::sin(x * 0.1f) * 5.0f + std::sin(x * 0.05f) * 10.0f + std::sin(x * 0.01f) * 50.0f;
}

// Get the color for a given height value based on terrain regions
void getColor(float height, float &red, float &green, float &blue)
{
    for (const auto &region : regions)
    {
        if (height >= region.minHeight && height <= region.maxHeight)
        {
            red = region.red;
            green = region.green;
            blue = region.blue;
            return;
        }
    }
}
// Function to generate height based on player position
GLfloat getHeight(GLfloat x, GLfloat z)
{
    // Define regions
    const GLfloat region1Start = 10.0f;
    const GLfloat region2Start = 50.0f;
    const GLfloat region3Start = 500.0f;

    // Define frequencies for sine functions
    const GLfloat freq1 = 0.1f;
    const GLfloat freq2 = 0.05f;
    const GLfloat freq3 = 0.01f;

    // Generate height based on regions
    if (z <= region1Start)
    {
        return std::sin(freq1 * x) * std::cos(freq1 * z);
    }
    else if (z <= region2Start)
    {
        return std::sin(freq2 * x) * std::cos(freq2 * z);
    }
    else
    {
        return std::sin(freq3 * x) * std::cos(freq3 * z);
    }
}

// Function to render the terrain
void renderTerrain(Player &player)
{
    // Loop through terrain grid
    for (GLfloat x = player.posX - WINDOW_WIDTH / 2; x < player.posX + WINDOW_WIDTH / 2; ++x)
    {
        for (GLfloat z = player.posZ - WINDOW_HEIGHT / 2; z < player.posZ + WINDOW_HEIGHT / 2; ++z)
        {
            // Get height at current position
            GLfloat height = getHeight(x, z);

            // Determine color based on height
            const GLfloat *color;
            if (height < 0.2f)
            {
                color = waterColor;
            }
            else if (height < 0.5f)
            {
                color = sandColor;
            }
            else
            {
                color = dirtColor;
            }

            // Render terrain quad
            glBegin(GL_QUADS);
            glColor3fv(color);
            glVertex3f(x, height, z);
            glVertex3f(x + 1, height, z);
            glVertex3f(x + 1, height, z + 1);
            glVertex3f(x, height, z + 1);
            glEnd();
        }
    }
}

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
    return 0;
}

// Function to render the scene
void renderScene(SDL_Window *window, Player &player, float deltaTime)
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    calculateViewMatrix();

    renderAxis();

    // Render ground plane
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glVertex3f(-10.0f, -GROUND_Y, -10.0f);
    glVertex3f(-10.0f, GROUND_Y, 10.0f);
    glVertex3f(10.0f, GROUND_Y, 10.0f);
    glVertex3f(10.0f, -GROUND_Y, -10.0f);
    // glEnd();
    // Render terrain
    // Render the terrain using triangle strips
    // glBegin(GL_TRIANGLE_STRIP);
    // for (float x = -500.0f; x < 500.0f; x += 10.0f) {
    //     for (float z = -500.0f; z < 500.0f; z += 10.0f) {
    //         // Calculate height for each vertex
    //         float height1 = generateHeight(x, z);
    //         float height2 = generateHeight(x, z + 10.0f);

    //         // Determine color based on height
    //         float red1, green1, blue1;
    //         float red2, green2, blue2;
    //         getColor(height1, red1, green1, blue1);
    //         getColor(height2, red2, green2, blue2);

    //         // Set color for the current vertex
    //         glColor3f(red1, green1, blue1);
    //         glVertex3f(x, height1, z);

    //         // Set color for the next vertex
    //         glColor3f(red2, green2, blue2);
    //         glVertex3f(x, height2, z + 10.0f);
    //     }
    // }

    // Render player
    glPushMatrix();
    drawStickman(player.posX, player.posY, player.posZ);
    // glTranslatef(player.posX, player.posY, player.posZ);
    // glRotatef(player.rotationY, 0.0f, 1.0f, 0.0f); // Rotate player around the Y-axis

    glPopMatrix();

    // // Render enemies
    // for (const auto &enemy : enemies)
    // {
    //     glPushMatrix();
    //     glTranslatef(enemy.posX, enemy.posY, enemy.posZ);
    //     glColor3f(1.0f, 0.0f, 0.0f);
    //     glBegin(GL_QUADS);
    //     glVertex3f(-0.25f, -0.25f, -0.25f);
    //     glVertex3f(0.25f, -0.25f, -0.25f);
    //     glVertex3f(0.25f, 0.25f, -0.25f);
    //     glVertex3f(-0.25f, 0.25f, -0.25f);
    //     glEnd();
    //     glPopMatrix();
    // }

    // // Render projectiles
    // for (const auto &projectile : projectiles)
    // {
    //     glPushMatrix();
    //     glTranslatef(projectile.posX, projectile.posY, projectile.posZ);
    //     glColor3f(0.0f, 1.0f, 0.0f);
    //     glBegin(GL_QUADS);
    //     glVertex3f(-0.1f, -0.1f, -0.1f);
    //     glVertex3f(0.1f, -0.1f, -0.1f);
    //     glVertex3f(0.1f, 0.1f, -0.1f);
    //     glVertex3f(-0.1f, 0.1f, -0.1f);
    //     glEnd();
    //     glPopMatrix();
    // }


    
    // Render the text to a texture if it hasn't been rendered yet
    if (!textTexture)
    {
        textTexture = renderText(renderer, font, "strapicarus god", textColor);
        if (!textTexture)
        {
            // Handle text rendering failure
            // Maybe retry or fall back to a default text
        }
        if (textTexture)
        {

            // // Get the OpenGL texture ID from the SDL_Texture
            // GLuint glTextureID;
            SDL_GL_BindTexture(textTexture, NULL, NULL); //, &glTextureID);

            // Set up orthogonal projection for 2D rendering
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            // Draw the textured quad
            glBegin(GL_QUADS);
            glTexCoord2f(0, 0);
            glVertex2f(1.0f, 1.0f);
            glTexCoord2f(1, 0);
            glVertex2f(1.0f + textWidth, 1.0f);
            glTexCoord2f(1, 1);
            glVertex2f(1.0f + textWidth, 1.0f + textHeight);
            glTexCoord2f(0, 1);
            glVertex2f(1.0f, 1.0f + textHeight);
            // glEnd();

            // Unbind the texture from the current OpenGL context
            SDL_GL_UnbindTexture(textTexture);
        }
    }
    glEnd();
    // Swap buffers
    SDL_GL_SwapWindow(window);
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
    Player player(0.0f, 0.0f, 0.0f);

    
    std::thread playlistThread([&]()
                               { playlist.addSongsFromDirectory("assets/media/music"); });

    

   
    Timer timer;
    bool quit = false;

    while (!quit)
    {
        float deltaTime = timer.GetDeltaTime();

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Handle input
        handleInput(window, player, deltaTime);
        checkKeyStatus(player, deltaTime);
        updateCameraPosition(player, deltaTime);
        updateEnemies();
        updateProjectiles();
        std::vector<PhysObject *> physObjects;
        physObjects.push_back(reinterpret_cast<PhysObject *>(&player));

        updatePhysics(physObjects, GRAVITY, deltaTime);
        setupProjectionMatrix();
        // Render the scene
        renderScene(window, player, deltaTime);

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
    TTF_Quit();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}