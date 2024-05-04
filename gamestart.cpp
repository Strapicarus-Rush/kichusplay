#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "freetype.h"
#include <SFML/Audio.hpp>

// Include tinygltf for GLTF model loading
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

// Include stb_image_write library for image writing
// #ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

sf::SoundBuffer b_music;
sf::Sound bp_music;

enum class GameState
{
    INITIAL,
    LOADING,
    MENU,
    GAMEPLAY
};

// Function to load an image using stb_image
unsigned char *loadImage(const char *filename, int &width, int &height, int &channels)
{
    stbi_set_flip_vertically_on_load(false); // Flip loaded image vertically to match OpenGL's coordinate system
    return stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
}

// Function to initialize GLFW and create a window
GLFWwindow *initWindow(int width, int height, const char *title)
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    // Set GLFW options (optional)
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

    // Create a windowed mode window and its OpenGL context
    GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Enable v-sync
    glfwSwapInterval(1);

    return window;
}

// void renderImage(unsigned char *data, int width, int height, int windowWidth, int windowHeight)
// {
//     glMatrixMode(GL_PROJECTION);
//     glLoadIdentity();
//     glOrtho(0, windowWidth, windowHeight, 0, -1, 1); // Set up orthographic projection

//     glMatrixMode(GL_MODELVIEW);
//     glLoadIdentity();

//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the framebuffer

//     // Calculate the size of the image based on the window size
//     int imageSize = std::min(windowWidth, windowHeight) / 2;

//     // Calculate the position to center the image
//     int x = (windowWidth - imageSize) / 2;
//     int y = (windowHeight - imageSize) / 2;

//     // Render the image as a textured quad
//     glEnable(GL_TEXTURE_2D);
//     GLuint texID;
//     glGenTextures(1, &texID);
//     glBindTexture(GL_TEXTURE_2D, texID);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

//     glBegin(GL_QUADS);
//     glTexCoord2f(0, 1);
//     glVertex2f(x, y);
//     glTexCoord2f(0, 1);
//     glVertex2f(x, y + imageSize);
//     glTexCoord2f(1, 1);
//     glVertex2f(x + imageSize, y + imageSize);
//     glTexCoord2f(1, 0);
//     glVertex2f(x + imageSize, y);
//     glEnd();

//     glDeleteTextures(1, &texID); // Clean up texture

//     glDisable(GL_TEXTURE_2D);
// }

void renderImage(unsigned char *data, int width, int height, int windowWidth, int windowHeight)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the framebuffer

    // Set up perspective projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float top = nearPlane * tan(fov * 0.5f * 3.14159265358979323846f / 180.0f);
    float bottom = -top;
    float right = top * aspectRatio;
    float left = -right;
    glFrustum(left, right, bottom, top, nearPlane, farPlane);

    // Set up camera position and orientation
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Define the camera transformation (position and orientation)
    // Example: viewing from (0, 0, 5) towards the origin with the up direction along the positive y-axis
    float cameraPosition[3] = {0.0f, 0.0f, 5.0f};
    float target[3] = {0.0f, 0.0f, 0.0f};
    float up[3] = {0.0f, 1.0f, 0.0f};

    // Calculate the view matrix
    float forward[3];
    for (int i = 0; i < 3; ++i) {
        forward[i] = target[i] - cameraPosition[i];
    }
    float forwardLength = sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    for (int i = 0; i < 3; ++i) {
        forward[i] /= forwardLength;
    }
    float side[3];
    side[0] = forward[1] * up[2] - forward[2] * up[1];
    side[1] = forward[2] * up[0] - forward[0] * up[2];
    side[2] = forward[0] * up[1] - forward[1] * up[0];
    float sideLength = sqrt(side[0] * side[0] + side[1] * side[1] + side[2] * side[2]);
    for (int i = 0; i < 3; ++i) {
        side[i] /= sideLength;
    }
    float newUp[3];
    newUp[0] = side[1] * forward[2] - side[2] * forward[1];
    newUp[1] = side[2] * forward[0] - side[0] * forward[2];
    newUp[2] = side[0] * forward[1] - side[1] * forward[0];
    float viewMatrix[16] = {side[0], newUp[0], -forward[0], 0.0f,
                            side[1], newUp[1], -forward[1], 0.0f,
                            side[2], newUp[2], -forward[2], 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f};
    glMultMatrixf(viewMatrix);
    
    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Enable texturing
    glEnable(GL_TEXTURE_2D);
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Calculate the size of the image based on the window size
    float imageSize = std::min(windowWidth, windowHeight) * 0.5f;

    // Render the image as a textured quad
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex3f(-imageSize / 2, -imageSize / 2, 0.0f);
    glTexCoord2f(0, 0);
    glVertex3f(-imageSize / 2, imageSize / 2, 0.0f);
    glTexCoord2f(1, 0);
    glVertex3f(imageSize / 2, imageSize / 2, 0.0f);
    glTexCoord2f(1, 1);
    glVertex3f(imageSize / 2, -imageSize / 2, 0.0f);
    glEnd();

    glDeleteTextures(1, &texID); // Clean up texture

    glDisable(GL_TEXTURE_2D);
}


// Function to handle mouse button events
void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Get cursor position
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Get window size
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // Check if cursor is inside the "Play" button
        int buttonWidth = 100;
        int buttonHeight = 50;
        int buttonX = (windowWidth - buttonWidth) / 2;
        int buttonY = (windowHeight - buttonHeight) / 2;
        if (xpos >= buttonX && xpos <= buttonX + buttonWidth && ypos >= buttonY && ypos <= buttonY + buttonHeight)
        {
            // Transition to gameplay state
            GameState gameState = GameState::GAMEPLAY;
        }
    }
}

void errorCallback(int error, const char *description)
{
    // fprintf(stderr, "Error: %s\n", description);
    std::cerr << "GLFW Error: " << description << std::endl;
}

int main() {
    // Set GLFW error callback
    glfwSetErrorCallback(errorCallback);

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Initialize FreeType and load font
    initFreeType("fonts-japanese-mincho.ttf"); // Assuming "arial.ttf" is the font file
    const int WIDTH = 1920;
    const int HEIGHT = 1080;
    const char* TITLE = "KichusPlay Game GOTY Delux Edition | By Strapicarus";

    // Initialize GLFW and create window
    GLFWwindow* window = initWindow(WIDTH, HEIGHT, TITLE);
    if (!window)
        return -1;

    // Load image
    int imageWidth, imageHeight, imageChannels;
    unsigned char* imageData = loadImage("logo.png", imageWidth, imageHeight, imageChannels);
    if (!imageData) {
        std::cerr << "Failed to load image" << std::endl;
        glfwTerminate();
        return -1;
    }

    double startTime = glfwGetTime();
    GameState gameState = GameState::LOADING;

    //Load Background Music
    if (!b_music.loadFromFile("Earl Grant - Fly Me To The Moon - side 1 - Decca DL 4454  D71EE DC Art Bass Vocal Boost-nr.flac"))
    {
        std::cerr << "Failed to load music" << std::endl;
        return -1;
    }

    bp_music.setBuffer(b_music);
    bp_music.play();
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Get elapsed time
        double currentTime = glfwGetTime();
        double elapsedTime = currentTime - startTime;

        // Get window size
        // int windowWidth, windowHeight;
        // glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

         // Set background color
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

        // Set viewport
        glViewport(0, 0, WIDTH, HEIGHT);

        if (gameState == GameState::LOADING)
        {
            // glClear(GL_COLOR_BUFFER_BIT);
            renderImage(imageData, imageWidth, imageHeight, WIDTH,HEIGHT);
            std::cerr << "Render image" << std::endl;
        }
        

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Sleep for a short duration to limit the frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 fps
    }

    // Clean up
    stbi_image_free(imageData);
    glfwTerminate();
    return 0;
}
