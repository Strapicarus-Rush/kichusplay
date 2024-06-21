
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
#include <numeric>   // For std::iota
#include "player.h"
#include "camera.h"

bool debug = false;
bool isMouseCaptured = false;
int RENDER_TERRAIN_SWITCH = 1;
constexpr float invRadiansToDegrees = 180.0f / M_PI;
constexpr float invDegreesToRadians = M_PI / 180.f;

const char *TITLE = "KichusPlay Game GOTY Delux Edition | By Strapicarus";
std::string fpsString;

// Define variables to track the state of each relevant key
bool keyWPressed = false;
bool keySPressed = false;
bool keyAPressed = false;
bool keyDPressed = false;
bool keyQPressed = false;
bool keyEPressed = false;
bool keySpacePressed = false;
bool keyLShifPressed = false;
bool mouseMotion = false;

const constexpr int WINDOW_WIDTH = 1920;
const constexpr int WINDOW_HEIGHT = 1080;

float dotProduct = 0.0f;

bool initializing = true;

float textWidth = 1.0f;
float textHeight = 1.0f;

const int SHADOW_WIDTH = 1024;
const int SHADOW_HEIGHT = 1024;

GLuint VBO, VAO;
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

int nrAttributes; // max vertex supported

const float TERRAIN_WIDTH = 8000.0f;
const float TERRAIN_LENGTH = 8000.0f;
const float TERRAIN_HEIGHT = 50.0f;
const float TERRAIN_MIN_RESOLUTION = 8.0f;
const float TERRAIN_MAX_RESOLUTION = 1.0f;
// float TERRAIN_RESOLUTION = 1.0f;
// float TERRAIN_SCALE = 0.008f;
float TERRAIN_RESOLUTION = 1.0f;
float TERRAIN_SCALE = 0.005f;
float startX;
float endX;
float startZ;
float endZ;

std::vector<std::vector<GLfloat>> HEIGHTMAP(TERRAIN_WIDTH, std::vector<GLfloat>(TERRAIN_LENGTH));
std::vector<std::vector<std::vector<GLfloat>>> NORMALS(TERRAIN_WIDTH, std::vector<std::vector<GLfloat>>(TERRAIN_LENGTH, std::vector<GLfloat>(3)));

Player player(4000.0f, 0.0f, 4000.0f);

Camera camera(3998.0f, 5.0f, 3998.0f);

Playlist playlist(debug);

// struct Vertex {
//     float x, y, z;
//     float r, g, b;
//     float nx, ny, nz;
// };

// std::vector<Vertex> vertices;

// void addQuad(float x, float z, float height, float red, float green, float blue,
//              float nx, float ny, float nz, int xIndex, int zIndex, int terrain_resolution) {
//     vertices.push_back({x, height, z, red, green, blue, nx, ny, nz});
//     vertices.push_back({x + TERRAIN_RESOLUTION, HEIGHTMAP[xIndex + terrain_resolution][zIndex], z, red, green, blue, nx, ny, nz});
//     vertices.push_back({x + TERRAIN_RESOLUTION, HEIGHTMAP[xIndex + terrain_resolution][zIndex + terrain_resolution], z + TERRAIN_RESOLUTION, red, green, blue, nx, ny, nz});
//     vertices.push_back({x, HEIGHTMAP[xIndex][zIndex + TERRAIN_RESOLUTION], z + TERRAIN_RESOLUTION, red, green, blue, nx, ny, nz});
// }

class Timer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_LastTime;

public:
    Timer()
    {
        m_LastTime = std::chrono::high_resolution_clock::now();
    }
    float fps = 0.0f;
    int frameCount = 0;
    void calculateFPS(float &deltatime)
    {
        // Increment frame count
        frameCount++;

        // If a second has passed
        if (deltatime >= 0.01f)
        {
            // Calculate FPS
            fps = frameCount / deltatime;

            // Reset frame count and timer
            frameCount = 0;
            // m_LastTime = currentTime;

            // Convert FPS to string
            std::ostringstream fpsStream;
            fpsStream.precision(2);
            fpsStream << std::fixed << "FPS: " << fps << " | Camera yaw:" << camera.getYaw() << " | camera pitch:" << camera.getPitch();
            // fpsStream << std::fixed << "FPS: " << fps << " | Payer roty|x|y|z:" << player.getRotY() << " | " << player.getPosX() << " | " << player.getPosY() << " | " << player.getPosZ() << " Camera rotY|x|y|z:" << camera.getYaw() << "|" << camera.getPosX() << "|" << camera.getPosY() << "|" << camera.getPosZ() << " DotProduct:" << dotProduct << " FRUSTUM sx|ex  " << startX << "|" << endX << "  |  " << startZ << "|" << endZ;
            fpsString = fpsStream.str();
        }
        // Update window title with FPS
        SDL_SetWindowTitle(window, fpsString.c_str());
    }

    float GetDeltaTime()
    {
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(now - m_LastTime).count();
        m_LastTime = now;
        return deltaTime;
    }
};

void setupProjectionMatrix()
{
    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glFrustum(camera.nearFrustumLeft, camera.nearFrustumRight, camera.nearFrustumBottom, camera.nearFrustumTop, camera.nearPlane, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

// // Function to calculate the view matrix
void calculateViewMatrix()
{
    // Apply rotation
    glRotatef(camera.getPitch(), 1.0f, 0.0f, 0.0f);
    glRotatef(camera.getYaw(), 0.0f, 1.0f, 0.0f);
    glRotatef(camera.getRoll(), 0.0f,0.0f,1.0f);
    // Apply translation
    glTranslatef(-camera.getPosX(), -camera.getPosY(), -camera.getPosZ());
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
            if (event.key.keysym.sym == SDLK_LSHIFT)
            {
                keyLShifPressed = true;
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
            if (event.key.keysym.sym == SDLK_LSHIFT)
            {
                keyLShifPressed = false;
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
                mouseMotion = true;
                float yawMotion = event.motion.xrel * 0.1f;
                float pitchMotion = event.motion.yrel * 0.1f;
                camera.updateRotation(yawMotion, pitchMotion);
            }
        }
    }
}
void checkKeyStatus(Player &player, float deltatime)
{
    const bool onAir = player.isOnAir();
    if (keyLShifPressed && !onAir)
    {
        player.run();
    }
    else if (!keyLShifPressed && !onAir)
    {
        player.walk();
    }
    if (keyWPressed)
    {
        player.moveForward(deltatime);
    }
    if (keySPressed)
    {
        player.moveBackwards(deltatime);
    }
    if (keyAPressed)
    {
        player.moveLeft(deltatime);
    }
    if (keyDPressed)
    {
        player.moveRigth(deltatime);
    }
    if (keySpacePressed && !player.isOnAir())
    {
        player.jump(deltatime);
    }
}
void renderNormals(float length, float posX, float posY, float posZ, GLfloat nx, GLfloat ny, GLfloat nz)
{

    // Render normals
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.7f, 0.7f); // Yellow color for normals
    // Calculate endpoint of the normal
    float endX = posX + nx * length;
    float endY = posY + ny * length;
    float endZ = posZ + nz * length;

    // Draw the normal line segment
    glVertex3f(posX, posY, posZ);
    glVertex3f(endX, endY, endZ);

    glEnd();
}
void drawSolidSphere(float radius, int numStacks, int numSlices)
{
    float nx,ny,nz;
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
            float z = -sin(phi);
            nx = x * r2 / radius;
            ny = y2 / radius;
            nz = z * r2 / radius;
            
            // glBegin(GL_TRIANGLE_STRIP);
            glNormal3f(-nx, -ny, -nz);
            glVertex3f(x * r2, y2, z * r2);
            glNormal3f(x * r1 / radius, y1 / radius, z * r1 / radius);
            glVertex3f(x * r1, y1, z * r1);
        }
            glEnd();
            // renderNormals(4.5f, player.getPosX(), player.getPosY(),player.getPosZ(), nx, ny, nz);
    }
}

void drawSolidCube(float size)
{
    float halfSize = size / 2.0f;  
    float nx, ny, nz = 0.0f;
    nz =1.0f;

    glBegin(GL_QUADS);

    // Front face
    glNormal3f(nx, ny, -nz);
    glVertex3f(-halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);


    // Back face
    glNormal3f(nx, ny, nz);
    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(-halfSize, halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, -halfSize);

ny=1.0f;
nz=0.0f;
    // Top face
    glNormal3f(nx, ny, nz);
    glVertex3f(-halfSize, halfSize, -halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);

    // Bottom face
    glNormal3f(nx, -ny, nz);
    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);
    glVertex3f(-halfSize, -halfSize, halfSize);

ny=0.0f;
nx=1.0f;
    // Right face
    glNormal3f(nx, ny, nz);
    glVertex3f(halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);

    // Left face
    glNormal3f(-nx, ny, nz);
    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(-halfSize, -halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, -halfSize);

    glEnd();
    renderNormals(4.5f, player.getPosX(), player.getPosY(),player.getPosZ(), nx, ny, nz);
}
void drawSolidCubeAny(float size, float x, float y, float z, float r, float b, float g)
{
    float halfSize = size / 2.0f;

    glBegin(GL_QUADS);

    glColor3f(1.0f, 0.9f, 0.6f);
    // Front face
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x - halfSize, y - halfSize, z + halfSize);
    glVertex3f(x + halfSize, y - halfSize, z + halfSize);
    glVertex3f(x + halfSize, y + halfSize, z + halfSize);
    glVertex3f(x - halfSize, y + halfSize, z + halfSize);

    glColor3f(r, b, g);
    // Back face
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(x - halfSize, y - halfSize, z - halfSize);
    glVertex3f(x - halfSize, y + halfSize, z - halfSize);
    glVertex3f(x + halfSize, y + halfSize, z - halfSize);
    glVertex3f(x + halfSize, y - halfSize, z - halfSize);

    // Top face
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(x - halfSize, y + halfSize, z - halfSize);
    glVertex3f(x - halfSize, y + halfSize, z + halfSize);
    glVertex3f(x + halfSize, y + halfSize, z + halfSize);
    glVertex3f(x + halfSize, y + halfSize, z - halfSize);

    // Bottom face
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(x - halfSize, y - halfSize, z - halfSize);
    glVertex3f(x + halfSize, y - halfSize, z - halfSize);
    glVertex3f(x + halfSize, y - halfSize, z + halfSize);
    glVertex3f(x - halfSize, y - halfSize, z + halfSize);

    // Right face
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(x + halfSize, y - halfSize, z - halfSize);
    glVertex3f(x + halfSize, y + halfSize, z - halfSize);
    glVertex3f(x + halfSize, y + halfSize, z + halfSize);
    glVertex3f(x + halfSize, y - halfSize, z + halfSize);

    // Left face
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(x - halfSize, y - halfSize, z - halfSize);
    glVertex3f(x - halfSize, y - halfSize, z + halfSize);
    glVertex3f(x - halfSize, y + halfSize, z + halfSize);
    glVertex3f(x - halfSize, y + halfSize, z - halfSize);

    glEnd();
}

// Function to draw the stickman with procedural animation
void drawStickman(Player &player, float &deltaTime)
{
    // Calculate animation parameters
    float armSwing = std::cos(3.0f*deltaTime * 2.0f) * 60.0f; // Swing arms 45 degrees
    float legSwing = std::cos(3.0f*deltaTime * 2.0f) * 45.0f; // Swing legs 30 degrees

    // Apply player's rotation
    glPushMatrix();
    glTranslatef(player.getPosX(), player.getPosY(), player.getPosZ());
    glRotatef(player.getRotY(), 0.0f, 1.0f, 0.0f);

    // Draw head (hair)
    glPushMatrix();
    glTranslatef(0.0f, 2.7f, 0.0f); // Adjust position to separate hair from skin
    glColor3f(0.1f, 0.1f, 0.1f);    // Almost black color for hair
    drawSolidSphere(0.2f, 8, 8);    // Adjust sphere radius as needed
    glPopMatrix();

    // Draw head (skin)
    glPushMatrix();
    glTranslatef(0.0f, 2.5f, 0.0f);
    glColor3f(1.0f, 0.8f, 0.6f); // Skin color
    drawSolidSphere(0.2f, 6, 6); // Adjust sphere radius as needed
    glPopMatrix();

    // Draw body
    glPushMatrix();
    glTranslatef(0.0f, 2.0f, 0.0f);
    glColor3f(0.6f, 0.8f, 0.6f); // Light green color for body
    drawSolidCube(0.2f);         // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw left arm
    glPushMatrix();
    glTranslatef(-0.3f, 2.3f, 0.0f);
    glRotatef(armSwing, 0.0f, 0.0f, 1.0f); // Apply arm swing animation
    glColor3f(1.0f, 0.8f, 0.6f);           // Skin color for arms
    glScalef(0.8f, 0.2f, 0.2f);            // Scale to adjust arm size
    drawSolidCube(1.0f);                   // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw right arm
    glPushMatrix();
    glTranslatef(0.3f, 2.3f, 0.0f);
    glRotatef(-armSwing, 0.0f, 1.0f, -1.0f); // Apply arm swing animation (opposite direction)
    glColor3f(1.0f, 0.8f, 0.6f);            // Skin color for arms
    glScalef(0.8f, 0.2f, 0.2f);             // Scale to adjust arm size
    drawSolidCube(1.0f);                    // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw hips
    glPushMatrix();
    glTranslatef(0.0f, 1.6f, 0.0f);
    glColor3f(0.0f, 0.5f, 0.75f); // Dark cyan color for hips
    glScalef(0.5f, 0.4f, 0.2f);   // Scale to adjust hip size
    drawSolidCube(1.0f);          // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw left leg
    glPushMatrix();
    glTranslatef(-0.1f, 1.0f, 0.0f);
    glRotatef(legSwing, 1.0f, 0.0f, 0.0f); // Apply leg swing animation
    glColor3f(0.0f, 0.5f, 0.75f);          // Dark cyan color for legs
    glScalef(0.4f, 0.8f, 0.2f);            // Scale to adjust leg size
    drawSolidCube(1.0f);                   // Adjust cube dimensions as needed
    glPopMatrix();

    // Draw right leg
    glPushMatrix();
    glTranslatef(0.1f, 1.0f, 0.0f);
    glRotatef(-legSwing, 1.0f, 0.0f, 0.0f); // Apply leg swing animation (opposite direction)
    glColor3f(0.0f, 0.5f, 0.75f);           // Dark cyan color for legs
    glScalef(0.4f, 0.8f, 0.2f);             // Scale to adjust leg size
    drawSolidCube(1.0f);                    // Adjust cube dimensions as needed
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

// void drawStickman(Player &player)
// {
//     // Apply player's rotation
//     glPushMatrix();
//     glTranslatef(player.getPosX(), player.getPosY(), player.getPosZ());
//     glRotatef(player.getRotY(), 0.0f, 1.0f, 0.0f);

//     // Draw head (hair)
//     glPushMatrix();
//     glTranslatef(0.0f, 2.7f, 0.0f); // Adjust position to separate hair from skin
//     glColor3f(0.1f, 0.1f, 0.1f);    // Almost black color for hair
//     drawSolidSphere(0.2f, 8, 8);    // Adjust sphere radius as needed
//     glPopMatrix();

//     // Draw head (skin)
//     glPushMatrix();
//     glTranslatef(0.0f, 2.5f, 0.0f);
//     glColor3f(1.0f, 0.8f, 0.6f); // Skin color
//     drawSolidSphere(0.2f, 6, 6); // Adjust sphere radius as needed
//     glPopMatrix();

//     // Draw body
//     glPushMatrix();
//     glTranslatef(0.0f, 2.0f, 0.0f);
//     glColor3f(0.6f, 0.8f, 0.6f); // Light green color for body
//     drawSolidCube(0.2f);         // Adjust cube dimensions as needed
//     glPopMatrix();

//     // Draw arms
//     glPushMatrix();
//     glTranslatef(-0.3f, 2.3f, 0.0f);
//     glColor3f(1.0f, 0.8f, 0.6f); // Skin color for arms
//     glScalef(0.8f, 0.2f, 0.2f);  // Scale to adjust arm size
//     drawSolidCube(1.0f);         // Adjust cube dimensions as needed
//     glPopMatrix();

//     glPushMatrix();
//     glTranslatef(0.3f, 2.3f, 0.0f);
//     glColor3f(1.0f, 0.8f, 0.6f); // Skin color for arms
//     glScalef(0.8f, 0.2f, 0.2f);  // Scale to adjust arm size
//     drawSolidCube(1.0f);         // Adjust cube dimensions as needed
//     glPopMatrix();

//     // Draw hips
//     glPushMatrix();
//     glTranslatef(0.0f, 1.6f, 0.0f);
//     glColor3f(0.0f, 0.5f, 0.75f); // Dark cyan color for hips
//     glScalef(0.5f, 0.4f, 0.2f);   // Scale to adjust hip size
//     drawSolidCube(1.0f);          // Adjust cube dimensions as needed
//     glPopMatrix();

//     // Draw legs
//     glPushMatrix();
//     glTranslatef(-0.1f, 1.0f, 0.0f);
//     glColor3f(0.0f, 0.5f, 0.75f); // Dark cyan color for legs
//     glScalef(0.4f, 0.8f, 0.2f);   // Scale to adjust leg size
//     drawSolidCube(1.0f);          // Adjust cube dimensions as needed
//     glPopMatrix();

//     glPushMatrix();
//     glTranslatef(0.1f, 1.0f, 0.0f);
//     glColor3f(0.0f, 0.5f, 0.75f); // Dark cyan color for legs
//     glScalef(0.4f, 0.8f, 0.2f);   // Scale to adjust leg size
//     drawSolidCube(1.0f);          // Adjust cube dimensions as needed
//     glPopMatrix();

//     // Draw shoes
//     glPushMatrix();
//     glTranslatef(-0.1f, 0.5f, 0.0f);
//     glColor3f(1.0f, 1.0f, 1.0f); // White color for shoes
//     glScalef(0.4f, 0.2f, 0.2f);  // Scale to adjust shoe size
//     drawSolidCube(1.0f);         // Adjust cube dimensions as needed
//     glPopMatrix();

//     glPushMatrix();
//     glTranslatef(0.1f, 0.5f, 0.0f);
//     glColor3f(1.0f, 1.0f, 1.0f); // White color for shoes
//     glScalef(0.4f, 0.2f, 0.2f);  // Scale to adjust shoe size
//     drawSolidCube(1.0f);         // Adjust cube dimensions as needed
//     glPopMatrix();

//     // Restore the previous transformation state
//     glPopMatrix();
// }

void renderAxis(float size, float posX, float posY, float posZ)
{
    // Render axes (x: red, y: green, z: blue)
    glBegin(GL_LINES);
    // X-axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(posX - size, posY, posZ);
    glVertex3f(posX + size, posY, posZ);
    // Arrowhead for X-axis
    glVertex3f(posX + size, posY, posZ);
    glVertex3f(posX + size - size * 0.1f, posY + size * 0.1f, posZ);
    glVertex3f(posX + size, posY, posZ);
    glVertex3f(posX + size - size * 0.1f, posY - size * 0.1f, posZ);
    // Y-axis (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(posX, posY - size, posZ);
    glVertex3f(posX, posY + size, posZ);
    // Arrowhead for Y-axis
    glVertex3f(posX, posY + size, posZ);
    glVertex3f(posX + size * 0.1f, posY + size - size * 0.1f, posZ);
    glVertex3f(posX, posY + size, posZ);
    glVertex3f(posX - size * 0.1f, posY + size - size * 0.1f, posZ);
    // Z-axis (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(posX, posY, posZ - size);
    glVertex3f(posX, posY, posZ + size);
    // Arrowhead for Z-axis
    glVertex3f(posX, posY, posZ + size);
    glVertex3f(posX, posY + size * 0.1f, posZ + size - size * 0.1f);
    glVertex3f(posX, posY, posZ + size);
    glVertex3f(posX, posY - size * 0.1f, posZ + size - size * 0.1f);

    glEnd();
}

void renderLineBounds(float startPosX, float startPosZ, float endPosX, float endPosZ, float height)
{
    // Render axes (x: red, y: green, z: blue)
    glBegin(GL_LINES);

    // X-axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    // glVertex3f(startPosX, height, endPosX);
    // glVertex3f(startPosZ, height, endPosZ);
    glVertex3f(startPosZ, height, endPosX);
    glVertex3f(startPosX, height, endPosZ);
    // glVertex3f(startPosX, height, endPosZ);
    // glVertex3f(startPosZ, height, endPosX);
    // glVertex3f(endPosZ, height, endPosX);
    // glVertex3f(endPosX, height, endPosZ);
    // glVertex3f(startPosX, height, startPosZ);
    // glVertex3f(endPosZ, height, startPosX);
    // glVertex3f(startPosZ, height, startPosZ);
    // glVertex3f(startPosX, height, endPosZ);
    glEnd();
}

void checkGLError()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error: " << error << std::endl;
    }
}

class PerlinNoise
{
public:
    PerlinNoise()
    {
        std::iota(p, p + 256, 0); // Fill p with values from 0 to 255
        std::default_random_engine engine(std::random_device{}());
        std::shuffle(p, p + 256, engine); // Shuffle using std::shuffle
        for (int i = 0; i < 256; ++i)
        {
            p[256 + i] = p[i];
        }
    }

    float noise(float x, float y, float z) const
    {
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

        return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)), lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
                    lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
                         lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                              grad(p[BB + 1], x - 1, y - 1, z - 1))));
    }

private:
    int p[512];

    float fade(float t) const
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    float lerp(float t, float a, float b) const
    {
        return a + t * (b - a);
    }

    float grad(int hash, float x, float y, float z) const
    {
        int h = hash & 3;
        float u = h < 2 ? x : y;
        float v = h < 2 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};

// Define regions and their properties
struct TerrainRegion
{
    float minHeight;
    float maxHeight;
    float red, green, blue; // Color components
};

std::vector<TerrainRegion> regions = {
    {0.0f, 0.0000001f, 0.0f, 0.0f, 1.0f}, // Blue - Water
    {0.1f, 4.0f, 1.0f, 1.0f, 0.0f},       // Yellowish - Sand
    {4.1f, 8.0f, 0.4f, 0.2f, 0.0f},       // Brown - Dirt
    {8.1f, 12.0f, 0.0f, 1.0f, 0.0f},      // Green - Grassland
    {12.1f, 20.0f, 0.0f, 0.5f, 0.0f},     // Dark Green - Valley
    {20.1f, 25.0f, 0.04f, 0.8f, 0.0f},
    {25.1f, 46.0f, 0.5f, 0.35f, 0.1f}, // Light Brown - Highlands
    {46.1f, 70.0f, 0.6f, 0.6f, 0.6f},  // Gray - Mountain
    {70.1f, 120.0f, 1.0f, 1.0f, 1.0f}  // White - Peak Snow
};

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

// Create a heightmap using Perlin noise
void createHeightmap(int width, int length, const PerlinNoise &noise)
{
    if (debug > 0)
    {
        std::cout << "createHeightMap..." << std::endl;
    }
    GLfloat nx, ny, nz;
    for (float x = 0; x < width; ++x)
    {
        for (float z = 0; z < length; ++z)
        {
            HEIGHTMAP[x][z] = std::max(0.0000001f, noise.noise(x * TERRAIN_SCALE, z * TERRAIN_SCALE, noise.noise(x * TERRAIN_SCALE, z * TERRAIN_SCALE, -1.0f)) * TERRAIN_HEIGHT);
        }
    }
    for (float x = 0; x < width; ++x)
    {
        for (float z = 0; z < length; ++z)
        {
            // calculate normals
            GLfloat hL = (x > 0) ? HEIGHTMAP[x - 1][z] : HEIGHTMAP[x][z];
            GLfloat hR = (x < TERRAIN_WIDTH - 1) ? HEIGHTMAP[x + 1][z] : HEIGHTMAP[x][z];
            GLfloat hD = (z > 0) ? HEIGHTMAP[x][z - 1] : HEIGHTMAP[x][z];
            GLfloat hU = (z < TERRAIN_LENGTH - 1) ? HEIGHTMAP[x][z + 1] : HEIGHTMAP[x][z];

            nx = (hL - hR) / (TERRAIN_RESOLUTION);
            ny = sqrt(nx * nx + nz * nz + 8.0f);
            nz = (hD - hU) / (TERRAIN_RESOLUTION);

            GLfloat length = sqrt(nx * nx + ny * ny + nz * nz);
            if (length > 0.0f)
            {
                nx /= length;
                ny /= length;
                nz /= length;
                NORMALS[x][z][0] = nx;
                NORMALS[x][z][1] = ny;
                NORMALS[x][z][2] = nz;
            }
        }
    }
}

bool isPointInFrustum(float x, float z, float nx1, float nz1, float nx2, float nz2, float fx1, float fz1, float fx2, float fz2)
{
    auto cross = [](float x1, float z1, float x2, float z2)
    {
        return x1 * z2 - x2 * z1;
    };

    // Vectors from the point to the frustum vertices
    float v1x = nx1 - x, v1z = nz1 - z;
    float v2x = nx2 - x, v2z = nz2 - z;
    float v3x = fx2 - x, v3z = fz2 - z;
    float v4x = fx1 - x, v4z = fz1 - z;

    // Cross products to check the relative orientation
    bool b1 = cross(v1x, v1z, v2x, v2z) > 0.0f;
    bool b2 = cross(v2x, v2z, v3x, v3z) > 0.0f;
    bool b3 = cross(v3x, v3z, v4x, v4z) > 0.0f;
    bool b4 = cross(v4x, v4z, v1x, v1z) > 0.0f;

    return (b1 == b2) && (b2 == b3) && (b3 == b4);
}

double calculateDistance(double x1, double z1, double x2, double z2)
{
    double deltaX = x2 - x1;
    double deltaZ = z2 - z1;
    return std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
}

void renderTerrain()
{
    renderAxis(5.0f, player.getPosX(), player.getPosY(), player.getPosZ());

    float cosYaw = std::cos(-camera.getYaw() * invDegreesToRadians);
    float sinYaw = std::sin(-camera.getYaw() * invDegreesToRadians);
    float cosPitch = std::cos(camera.getPitch() * invDegreesToRadians);
    float sinPitch = std::sin(camera.getPitch() * invDegreesToRadians);

    float forwardX = cosYaw * camera.nearFrustumTop;
    float forwardZ = sinYaw * camera.nearFrustumTop;

    float nearCenterX = player.getPosX() + forwardZ;
    float nearCenterZ = player.getPosZ() + forwardX;
    drawSolidCubeAny(0.2f, nearCenterX, player.getPosY() + 3.5f, nearCenterZ, 1.0f, 0.0f, 0.0f);

    float nearTopLeftX = nearCenterX + cosYaw * camera.nearFrustumLeft;
    float nearTopLeftZ = nearCenterZ - sinYaw * camera.nearFrustumLeft;
    drawSolidCubeAny(0.2f, nearTopLeftX, player.getPosY() + 3.5f, nearTopLeftZ, 0.2, 1.0f, 0.1f);

    float nearBottomRightX = nearCenterX - cosYaw * -camera.nearFrustumRight;
    float nearBottomRightZ = nearCenterZ + sinYaw * -camera.nearFrustumRight;
    drawSolidCubeAny(0.2f, nearBottomRightX, player.getPosY() + 3.5f, nearBottomRightZ, 0.2f, 0.5f, 1.0f);

    forwardX = cosYaw * camera.farFrustumTop;
    forwardZ = sinYaw * camera.farFrustumTop;

    float farCenterX = camera.getPosX() - forwardZ;
    float farCenterZ = camera.getPosZ() - forwardX;
    drawSolidCubeAny(1.0f, farCenterX, player.getPosY() + 3.5f, farCenterZ, 1.0f, 0.0f, 0.0f);

    float farTopLeftX = farCenterX + cosYaw * camera.farFrustumLeft / 2.0f;
    float farTopLeftZ = farCenterZ - sinYaw * camera.farFrustumLeft / 2.0f;
    drawSolidCubeAny(0.5f, farTopLeftX, player.getPosY() + 3.5f, farTopLeftZ, 0.2f, 1.0f, 0.1f);

    float farBottomRightX = farCenterX - cosYaw * -camera.farFrustumRight / 2.0f;
    float farBottomRightZ = farCenterZ + sinYaw * -camera.farFrustumRight / 2.0f;
    drawSolidCubeAny(0.5f, farBottomRightX, player.getPosY() + 3.5f, farBottomRightZ, 0.2f, 0.5f, 1.0f);

    startX = std::max(0.0f, std::min(std::min({nearTopLeftX, nearBottomRightX, farTopLeftX, farBottomRightX}), TERRAIN_WIDTH - 1));
    endX = std::max(0.0f, std::min(std::max({nearTopLeftX, nearBottomRightX, farTopLeftX, farBottomRightX}), TERRAIN_WIDTH - 1));
    startZ = std::max(0.0f, std::min(std::min({nearTopLeftZ, nearBottomRightZ, farTopLeftZ, farBottomRightZ}), TERRAIN_LENGTH - 1));
    endZ = std::max(0.0f, std::min(std::max({nearTopLeftZ, nearBottomRightZ, farTopLeftZ, farBottomRightZ}), TERRAIN_LENGTH - 1));

    // float viewDirX = cosPitch * sinYaw;
    // float viewDirY = -sinPitch;
    // float viewDirZ = cosPitch * cosYaw;

    // GLuint query;

    // glGenQueries(1, &query);
    if (debug > 0)
    {
        std::cout << "renderTerrain..." << std::endl;
    }

    for (float x = startX; x < endX; x += TERRAIN_RESOLUTION)
    {
        for (float z = startZ; z < endZ; z += TERRAIN_RESOLUTION)
        {
            if (!isPointInFrustum(x, z, nearTopLeftX, nearTopLeftZ, nearBottomRightX, nearBottomRightZ, farTopLeftX, farTopLeftZ, farBottomRightX, farBottomRightZ))
            {
                continue;
            }
            float red, green, blue;
            // Get height at current position from the HEIGHTMAP
            int xIndex = static_cast<int>(x);
            int zIndex = static_cast<int>(z);
            int terrain_resolution = static_cast<int>(TERRAIN_RESOLUTION);
            GLfloat height = HEIGHTMAP[x][z];
            GLfloat nx = NORMALS[x][z][0];
            GLfloat ny = NORMALS[x][z][1];
            GLfloat nz = NORMALS[x][z][2];
            getColor(height, red, green, blue);
            glNormal3f(nx, ny, nz);

            // // // Use an occlusion query to determine visibility
            // glBeginQuery(GL_SAMPLES_PASSED, query);
            // // Render bounding box of the terrain patch for occlusion query
            // glBegin(GL_QUADS);
            // // glColor3f(0.0, 0.0, 0.0);
            // glVertex3f(x, height, z);
            // glVertex3f(x + TERRAIN_RESOLUTION, HEIGHTMAP[xIndex + terrain_resolution][zIndex], z);
            // glVertex3f(x + TERRAIN_RESOLUTION, HEIGHTMAP[xIndex + terrain_resolution][zIndex + terrain_resolution], z + TERRAIN_RESOLUTION);
            // glVertex3f(x, HEIGHTMAP[xIndex][zIndex + terrain_resolution], z + TERRAIN_RESOLUTION);
            // glEnd();

            // glEndQuery(GL_SAMPLES_PASSED);

            // GLuint sampleCount;
            // glGetQueryObjectuiv(query, GL_QUERY_RESULT, &sampleCount);
            // std::cout << "SampleCount" << sampleCount << std::endl;
            // If samples passed is zero, skip rendering this patch
            // if (sampleCount > 0)
            // {
            // std::cout << "culling terrain..." << std::endl;
            glBegin(GL_QUADS);

            glColor3f(red, green, blue);
            glVertex3f(x, height, z);
            glVertex3f(x + TERRAIN_RESOLUTION, HEIGHTMAP[xIndex + terrain_resolution][zIndex], z);
            glVertex3f(x + TERRAIN_RESOLUTION, HEIGHTMAP[xIndex + terrain_resolution][zIndex + terrain_resolution], z + TERRAIN_RESOLUTION);
            glVertex3f(x, HEIGHTMAP[xIndex][zIndex + terrain_resolution], z + TERRAIN_RESOLUTION);
            glEnd();
            continue;
            // }
            // renderNormals(2.5f, x, height, z, nx, ny, nz);
        }
    }
    // glDeleteQueries(1, &query);
}

void renderScene(SDL_Window *window, Player &player, float &deltaTime, const std::vector<std::vector<GLfloat>> &heightmap)
{
    if (debug > 0)
    {
        std::cout << "RenderScene..." << std::endl;
    }
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    calculateViewMatrix();

    // Render the axis
    // renderAxis(3.0f, player.getPosX(), player.getPosY(), player.getPosZ());

    // Render terrain
    renderTerrain();

    // Render player
    // glPushMatrix();
    drawStickman(player, deltaTime);
    // glPopMatrix();

    // Swap buffers
    SDL_GL_SwapWindow(window);
}
// void initialize_render() {
//     // Generate and bind the VAO
//     glGenVertexArrays(1, &VAO);
//     glBindVertexArray(VAO);

//     // Generate and bind the VBO
//     glGenBuffers(1, &VBO);
//     glBindBuffer(GL_ARRAY_BUFFER, VBO);
//     glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

//     // Vertex positions
//     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
//     glEnableVertexAttribArray(0);

//     // Vertex colors
//     glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, r)));
//     glEnableVertexAttribArray(1);

//     // Vertex normals
//     glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, nx)));
//     glEnableVertexAttribArray(2);

//     // Unbind the VBO and VAO
//     glBindBuffer(GL_ARRAY_BUFFER, 0);
//     glBindVertexArray(0);
// }

// void renderBuffer() {
//     glBindVertexArray(VAO);
//     glDrawArrays(GL_QUADS, 0, vertices.size());
//     glBindVertexArray(0);
// }

int initializeO_S(int argc, char *argv[])
{

    if (argc > 1 && argv[1] == "-debug")
    {
        debug = true;
        std::cout << "Debugging..." << std::endl;
    }
    if (debug > 0)
        std::cout << "Initialization..." << std::endl;
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
    PerlinNoise noise;
    createHeightmap(TERRAIN_WIDTH, TERRAIN_LENGTH, noise);
    // Set up OpenGL
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glEnable(GL_DEPTH_TEST); // Enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW); // clockwise winding is front face
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    Timer timer;
    bool quit = false;
    initializeO_S(argc, argv);
    // float deltaTime = timer.GetDeltaTime();
    // std::thread playlistThread([&]()
    //                            { playlist.addSongsFromDirectory("assets/media/music"); });

    while (!quit)
    {
        // if (event.type == SDL_WINDOWEVENT)
        // {
        //     if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        //     {
        //         WINDOW_WIDTH = event.window.data1;
        //         WINDOW_HEIGHT = event.window.data2;
        //     }
        // }
        float deltaTime = timer.GetDeltaTime();
        float playerHeight = HEIGHTMAP[static_cast<int>(floor(player.getPosX()))][static_cast<int>(floor(player.getPosZ()))];
        float cameraHeight = HEIGHTMAP[static_cast<int>(floor(camera.getPosX()))][static_cast<int>(floor(camera.getPosZ()))];
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        handleInput(window, player, deltaTime);
        checkKeyStatus(player, deltaTime);
        camera.updateCameraPosition(player.getPosX(), player.getPosY(), player.getPosZ(), deltaTime, cameraHeight);
        player.updateVerticalPosition(deltaTime, playerHeight, camera.getYaw());
        setupProjectionMatrix();
        renderScene(window, player, deltaTime, HEIGHTMAP);
        timer.calculateFPS(deltaTime);
        // if (playlist.loading == 0 && initializing == 1)
        // {
        //     if (debug > 0)
        //     {
        //         std::cout << "Playlist Loading: " << playlist.loading << std::endl;
        //     }

        //     playlistThread.join();
        //     playlist.play();
        //     initializing = false;
        // }
        // else if (playlist.loading == 0 && initializing == 0)
        // {
        //     playlist.playNextIfFinished();
        // }
        // if (RENDER_TERRAIN_SWITCH)
        // {
        //     /* code */
        // }
        // const std::chrono::milliseconds targetFrameDuration(1000 / 60);
        // auto waitTime=targetFrameDuration - deltaTime;
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 fps
    }
    TTF_Quit();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
//  git add README.md camera.h camera.cpp playlist.cpp player.h player.cpp