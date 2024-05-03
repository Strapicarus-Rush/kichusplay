#include <GLFW/glfw3.h>

void renderMenu(unsigned char* data, int width, int height, int windowWidth, int windowHeight, double elapsedTime) {
    // Calculate alpha value for fade in/out effect
    float alpha = 1.0f;
    if (elapsedTime < 2.5) {
        alpha = static_cast<float>(elapsedTime) / 2.5f; // Fade in during first 2.5 seconds
    } else if (elapsedTime > 2.5 && elapsedTime < 5.0) {
        alpha = 1.0f - static_cast<float>(elapsedTime - 2.5) / 2.5f; // Fade out during last 2.5 seconds
    }

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

    // Render the image as a textured quad with alpha value for fade effect
    glEnable(GL_TEXTURE_2D);
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glColor4f(1.0f, 1.0f, 1.0f, alpha); // Set alpha value for the image

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y);
    glTexCoord2f(-1, 1); glVertex2f(x + imageSize, y);
    glTexCoord2f(1, 1); glVertex2f(x + imageSize, y + imageSize);
    glTexCoord2f(1, -1); glVertex2f(x, y + imageSize);
    glEnd();

    glDeleteTextures(1, &texID); // Clean up texture

    glDisable(GL_TEXTURE_2D);
}
