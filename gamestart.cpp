#include <GL/glew.h>
#include <iostream>
#include <SDL.h>
#include <SDL_opengl.h>
#include <vector>
#include <cmath>
#include <SFML/Audio.hpp>
// #include <SDL_opengl_glext.h>
// #include <GL/glcorearb.h>

sf::SoundBuffer b_music;
sf::Sound bp_music;

const int SHADOW_WIDTH = 1024;
const int SHADOW_HEIGHT = 1024;
GLuint depthMapFBO;
GLuint shadowShaderProgram;
GLint lightSpaceMatrixLocation;
GLfloat lightSpaceMatrix[16];
GLuint depthMap;

void renderSceneFromLight();
void renderSceneWithShadows();

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

float DELTATIME = 0.04f; // Adjust as needed

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
float cameraPosY = 2.0f;
float cameraPosZ = 4.0f;
float cameraYaw = -90.0f; // Yaw angle (horizontal)
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
    glRotatef(cameraYaw, 0.0f, 1.0f, 0.0f);
    // Apply translation
    glTranslatef(-cameraPosX, -cameraPosY, -cameraPosZ);
}

// Function to handle keyboard and mouse input for camera movement and rotation
void handleInput(SDL_Window *window, Player &player)
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

void drawStickman(float posX, float posY, float posZ)
{
    // Set drawing color
    glColor3f(1.0f, 0.0f, 0.0f); // Red color

    // Draw head
    glPushMatrix();
    glTranslatef(posX, posY + 1.8f, posZ);
    glBegin(GL_POLYGON);
    const int numSegments = 30;
    for (int i = 0; i < numSegments; ++i)
    {
        float angle = 2.0f * M_PI * i / numSegments;
        glVertex2f(posX + cos(angle), posY + sin(angle));
    }
    glEnd();
    glPopMatrix();

    // Draw body
    glPushMatrix();
    glTranslatef(posX, posY + 0.5f, posZ);
    glBegin(GL_POLYGON);
    glVertex2f(posX - 0.2f, posY);
    glVertex2f(posX + 0.2f, posY);
    glVertex2f(posX + 0.2f, posY + 3.0f);
    glVertex2f(posX - 0.2f, posY + 3.0f);
    glEnd();
    glPopMatrix();

    // Draw arms
    glPushMatrix();
    glTranslatef(posX - 0.3f, posY + 1.0f, posZ);
    glScalef(0.8f, 0.2f, 1.0f); // Scale to adjust arm size
    glBegin(GL_POLYGON);
    glVertex2f(posX, posY);
    glVertex2f(posX + 1.0f, posY);
    glVertex2f(posX + 1.0f, posY + 1.0f);
    glVertex2f(posX, posY + 1.0f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(posX + 0.3f, posY + 1.0f, posZ);
    glScalef(0.8f, 0.2f, 1.0f); // Scale to adjust arm size
    glBegin(GL_POLYGON);
    glVertex2f(posX, posY);
    glVertex2f(posX + 1.0f, posY);
    glVertex2f(posX + 1.0f, posY + 1.0f);
    glVertex2f(posX, posY + 1.0f);
    glEnd();
    glPopMatrix();

    // Draw legs
    glPushMatrix();
    glTranslatef(posX - 0.1f, posY - 0.5f, posZ);
    glScalef(0.4f, 0.8f, 1.0f); // Scale to adjust leg size
    glBegin(GL_POLYGON);
    glVertex2f(posX, posY);
    glVertex2f(posX + 1.0f, posY);
    glVertex2f(posX + 1.0f, posY + 2.0f);
    glVertex2f(posX, posY + 2.0f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(posX + 0.1f, posY - 0.5f, posZ);
    glScalef(0.4f, 0.8f, 1.0f); // Scale to adjust leg size
    glBegin(GL_POLYGON);
    glVertex2f(posX, posY);
    glVertex2f(posX + 1.0f, posY);
    glVertex2f(posX + 1.0f, posY + 2.0f);
    glVertex2f(posX, posY + 2.0f);
    glEnd();
    glPopMatrix();
}

// Function to render the scene
void renderScene(SDL_Window *window, Player &player)
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    calculateViewMatrix();

    // Render axes (x: red, y: green, z: blue)
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); // X-axis (red)
    glVertex3f(-10.0f, 0.0f, 0.0f);
    glVertex3f(10.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f); // Y-axis (green)
    glVertex3f(0.0f, -10.0f, 0.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 0.0f); // Z-axis (blue)
    glVertex3f(0.0f, 0.0f, -10.0f);
    glVertex3f(0.0f, 0.0f, 10.0f);
    glEnd();

    // Render ground plane
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glVertex3f(-10.0f, -GROUND_Y, -10.0f);
    glVertex3f(-10.0f, GROUND_Y, 10.0f);
    glVertex3f(10.0f, GROUND_Y, 10.0f);
    glVertex3f(10.0f, -GROUND_Y, -10.0f);
    ;
    glEnd();

    // Render player
    glPushMatrix();
    drawStickman(player.posX, player.posY, player.posZ);
    // glTranslatef(player.posX, player.posY, player.posZ);
    // glRotatef(player.rotationY, 0.0f, 1.0f, 0.0f); // Rotate player around the Y-axis

    glPopMatrix();

    // Render enemies
    for (const auto &enemy : enemies)
    {
        glPushMatrix();
        glTranslatef(enemy.posX, enemy.posY, enemy.posZ);
        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex3f(-0.25f, -0.25f, -0.25f);
        glVertex3f(0.25f, -0.25f, -0.25f);
        glVertex3f(0.25f, 0.25f, -0.25f);
        glVertex3f(-0.25f, 0.25f, -0.25f);
        glEnd();
        glPopMatrix();
    }

    // Render projectiles
    for (const auto &projectile : projectiles)
    {
        glPushMatrix();
        glTranslatef(projectile.posX, projectile.posY, projectile.posZ);
        glColor3f(0.0f, 1.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex3f(-0.1f, -0.1f, -0.1f);
        glVertex3f(0.1f, -0.1f, -0.1f);
        glVertex3f(0.1f, 0.1f, -0.1f);
        glVertex3f(-0.1f, 0.1f, -0.1f);
        glEnd();
        glPopMatrix();
    }

    // Swap buffers
    SDL_GL_SwapWindow(window);
}

// Function to update camera position following the player with acceleration and deceleration
void updateCameraPosition(Player &player)
{
    float cameraSpeed = 0.2f; // Ajusta la velocidad según sea necesario
    float distance = 5.0f;    // Distancia desde la parte trasera del jugador
    float height = 2.0f;      // Altura sobre el jugador

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
    float targetX = player.posX - directionX;
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
void updatePhysics(std::vector<PhysObject *> &physObjects, float gravity)
{

    // Update vertical position of each object
    for (auto &obj : physObjects)
    {
        obj->updateVerticalPosition(DELTATIME, gravity);
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

int main(int argc, char *argv[])
{

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    // Create SDL window
    SDL_Window *window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Create SDL OpenGL context
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context)
    {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Set up OpenGL
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Initialize enemies
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;

    // Create a player instance
    Player player(0.0f, 0.0f, 2.0f);

    // Load Background Music
    if (!b_music.loadFromFile("Earl Grant - Fly Me To The Moon - side 1 - Decca DL 4454  D71EE DC Art Bass Vocal Boost-nr.flac"))
    {
        std::cerr << "Failed to load music" << std::endl;
        return -1;
    }

    bp_music.setBuffer(b_music);
    bp_music.play();

    // Main loop
    bool quit = false;
    while (!quit)
    {
        // Handle input
        handleInput(window, player);

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
            player.posY += player.velocityY * DELTATIME;
        }
        else
        {
            player.velocityY -= GRAVITY;
            player.posY += player.velocityY * DELTATIME;
        }
        updateCameraPosition(player);
        updateEnemies();
        updateProjectiles();
        std::vector<PhysObject *> physObjects;
        physObjects.push_back(reinterpret_cast<PhysObject *>(&player));
        for (auto &enemy : enemies)
        {
            physObjects.push_back(reinterpret_cast<PhysObject *>(&enemy));
        }
        for (auto &projectile : projectiles)
        {
            physObjects.push_back(reinterpret_cast<PhysObject *>(&projectile));
        }
        updatePhysics(physObjects, GRAVITY);
        setupProjectionMatrix();
        // Render the scene
        renderScene(window, player);

        // Delay to control frame rate
        SDL_Delay(10);
    }

    // Cleanup
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}