#include <GLFW/glfw3.h>
#include <SFML/Audio.hpp>
#include <GL/gl.h>
// #include <GL/glut.h> DO NOT USE GLUT
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "menu.h"
#include "freetype.h"

// FT_Face face;
// Include stb_image library for image loading
// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"

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

enum class GameState
{
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

// void renderText(const char *text, float x, float y, float scale)
// {
//     glMatrixMode(GL_PROJECTION);
//     glPushMatrix();
//     glLoadIdentity();
//     glOrtho(0.0f, 100, 50, 0.0f, -1.0f, 1.0f);

//     glMatrixMode(GL_MODELVIEW);
//     glPushMatrix();
//     glLoadIdentity();

//     // Set text color
//     glColor3f(1.0f, 1.0f, 1.0f); // White color for text

//     // Set position and scale
//     glTranslatef(x, y, 0.0f);
//     glScalef(scale, scale, 1.0f);

//     // Render each character
//     while (*text)
//     {
//         glfwBitmapCharacter(GLFW_BITMAP_TIMES_ROMAN_24, *text);
//         ++text;
//     }

//     glPopMatrix();
//     glMatrixMode(GL_PROJECTION);
//     glPopMatrix();
//     glMatrixMode(GL_MODELVIEW);
// }

void renderPlayButton(int windowWidth, int windowHeight) {
    // Render the "Play" button at the center of the screen
    int buttonWidth = 100;
    int buttonHeight = 50;
    int buttonX = (windowWidth - buttonWidth) / 2;
    int buttonY = (windowHeight - buttonHeight) / 2;

    glColor3f(0.0f, 0.0f, 1.0f); // Blue color for the button
    glBegin(GL_QUADS);
    glVertex2f(buttonX, buttonY);
    glVertex2f(buttonX + buttonWidth, buttonY);
    glVertex2f(buttonX + buttonWidth, buttonY + buttonHeight);
    glVertex2f(buttonX, buttonY + buttonHeight);
    glEnd();

    // Render the text "Play" inside the button using GLFW's built-in bitmap font
    glColor3f(1.0f, 1.0f, 1.0f);                           // White color for the text
    float textPosX = buttonX + (buttonWidth - 50) / 2.0f;  // Adjust position for centering
    float textPosY = buttonY + (buttonHeight + 12) / 2.0f; // Adjust position for centering
    renderText(face, "Hello, World!", 100.0f, 100.0f, 1.0f, 1.0f, 0.0f, 0.0f);    // Pass 'face' as the first argument
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
    // glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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

// Function declarations
// void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

void renderImage(unsigned char *data, int width, int height, int windowWidth, int windowHeight)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1); // Set up orthographic projection with origin at top-left

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the framebuffer

    // Calculate the size of the image based on the window size
    int imageSize = std::min(windowWidth, windowHeight) / 2;

    // Calculate the position to center the image
    int x = (windowWidth - imageSize) / 2;
    int y = (windowHeight - imageSize) / 2;

    // Render the image as a textured quad
    glEnable(GL_TEXTURE_2D);
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(x, y);
    glTexCoord2f(-1, 1);
    glVertex2f(x + imageSize, y);
    glTexCoord2f(1, 1);
    glVertex2f(x + imageSize, y + imageSize);
    glTexCoord2f(1, -1);
    glVertex2f(x, y + imageSize);
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

// Function to render a GLTF model
void renderModel(const tinygltf::Model &model)
{
    // Render the GLTF model
    // You need to implement rendering of the GLTF model based on the loaded data
    // This may involve rendering meshes, materials, and textures from the model
    // Consult the documentation of your GLTF loader library for details on rendering
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

    // Required minimum OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    const int WIDTH = 800;
    const int HEIGHT = 600;
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
    GameState gameState = GameState::MENU;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Get elapsed time
        double currentTime = glfwGetTime();
        double elapsedTime = currentTime - startTime;

        // Get window size
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // Render text
        float textColor[3] = {1.0f, 0.0f, 0.0f}; // Red color
        renderText(face, "Hello, World!", 100.0f, 100.0f, 1.0f, 1.0f, 0.0f, 0.0f);

        // Render based on current game state
        switch (gameState) {
            case GameState::LOADING:
                // renderLoading(imageData, imageWidth, imageHeight, windowWidth, windowHeight, elapsedTime);
                break;
            case GameState::MENU:
                renderMenu(imageData, imageWidth, imageHeight, windowWidth, windowHeight, elapsedTime);
                break;
            case GameState::GAMEPLAY:
                // Render gameplay
                break;
            // Add other game states as needed...
        }

        // Render "Play" button if in menu state
        if (gameState == GameState::MENU) {
            renderPlayButton(windowWidth, windowHeight);
        }

        // Render image
        glClear(GL_COLOR_BUFFER_BIT);
        renderImage(imageData, imageWidth, imageHeight, windowWidth, windowHeight);

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
