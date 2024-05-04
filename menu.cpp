#include "menu.h"
#include <GL/gl.h>
#include <SFML/Audio.hpp>

sf::SoundBuffer buffer;
sf::Sound sound;
void renderMenu(int windowWidth, int windowHeight)
{
    // Calculate the position and size of the buttons
    int buttonWidth = 100;
    int buttonHeight = 50;
    int playButtonX = (windowWidth - buttonWidth) / 2;
    int playButtonY = (windowHeight - buttonHeight) / 3;
    int quitButtonX = (windowWidth - buttonWidth) / 2;
    int quitButtonY = (windowHeight - buttonHeight) * 2 / 3;

    // Render the play button
    glColor3f(0.0f, 0.0f, 1.0f); // Blue color for the button
    glBegin(GL_QUADS);
    glVertex2f(playButtonX, playButtonY);
    glVertex2f(playButtonX + buttonWidth, playButtonY);
    glVertex2f(playButtonX + buttonWidth, playButtonY + buttonHeight);
    glVertex2f(playButtonX, playButtonY + buttonHeight);
    glEnd();

    // Render the quit button
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for the button
    glBegin(GL_QUADS);
    glVertex2f(quitButtonX, quitButtonY);
    glVertex2f(quitButtonX + buttonWidth, quitButtonY);
    glVertex2f(quitButtonX + buttonWidth, quitButtonY + buttonHeight);
    glVertex2f(quitButtonX, quitButtonY + buttonHeight);
    glEnd();

    // Play background music
    
    if (buffer.loadFromFile("Earl Grant - Fly Me To The Moon - side 1 - Decca DL 4454  D71EE DC Art Bass Vocal Boost-nr.flac"))
    {
        // music.setLoop(true);
        
        sound.setBuffer(buffer);
        sound.play();   
    }
}


// #include "menu.h"
// #include <GL/gl.h>
// #include <SFML/Audio.hpp>
// #include <GLFW/glfw3.h>

// void renderMenu(int windowWidth, int windowHeight)
// {
//     // Calculate alpha value for fade in/out effect
//     float alpha = 1.0f;
//     if (elapsedTime < 2.5) {
//         alpha = static_cast<float>(elapsedTime) / 2.5f; // Fade in during first 2.5 seconds
//     } else if (elapsedTime > 2.5 && elapsedTime < 5.0) {
//         alpha = 1.0f - static_cast<float>(elapsedTime - 2.5) / 2.5f; // Fade out during last 2.5 seconds
//     }

//     glMatrixMode(GL_PROJECTION);
//     glLoadIdentity();
//     glOrtho(0, windowWidth, windowHeight, 0, -1, 1); // Set up orthographic projection with origin at top-left

//     glMatrixMode(GL_MODELVIEW);
//     glLoadIdentity();

//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the framebuffer

//     // Render the play button as a textured quad with alpha value for fade effect
//     glEnable(GL_TEXTURE_2D);
//     GLuint playButtonTexID;
//     glGenTextures(1, &playButtonTexID);
//     glBindTexture(GL_TEXTURE_2D, playButtonTexID);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, playButtonWidth, playButtonHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, playButtonData);

//     glColor4f(1.0f, 1.0f, 1.0f, alpha); // Set alpha value for the play button
//     // Adjust position and size of the play button
//     int playButtonX = (windowWidth - playButtonWidth) / 2;
//     int playButtonY = (windowHeight - playButtonHeight) / 3;
//     glBegin(GL_QUADS);
//     glTexCoord2f(0, 0); glVertex2f(playButtonX, playButtonY);
//     glTexCoord2f(1, 0); glVertex2f(playButtonX + playButtonWidth, playButtonY);
//     glTexCoord2f(1, 1); glVertex2f(playButtonX + playButtonWidth, playButtonY + playButtonHeight);
//     glTexCoord2f(0, 1); glVertex2f(playButtonX, playButtonY + playButtonHeight);
//     glEnd();
//     glDeleteTextures(1, &playButtonTexID); // Clean up texture

//     // Render the quit button as a textured quad with alpha value for fade effect
//     GLuint quitButtonTexID;
//     glGenTextures(1, &quitButtonTexID);
//     glBindTexture(GL_TEXTURE_2D, quitButtonTexID);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, quitButtonWidth, quitButtonHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, quitButtonData);

//     // Adjust position and size of the quit button
//     int quitButtonX = (windowWidth - quitButtonWidth) / 2;
//     int quitButtonY = (windowHeight - quitButtonHeight) * 2 / 3;
//     glColor4f(1.0f, 1.0f, 1.0f, alpha); // Set alpha value for the quit button
//     glBegin(GL_QUADS);
//     glTexCoord2f(0, 0); glVertex2f(quitButtonX, quitButtonY);
//     glTexCoord2f(1, 0); glVertex2f(quitButtonX + quitButtonWidth, quitButtonY);
//     glTexCoord2f(1, 1); glVertex2f(quitButtonX + quitButtonWidth, quitButtonY + quitButtonHeight);
//     glTexCoord2f(0, 1); glVertex2f(quitButtonX, quitButtonY + quitButtonHeight);
//     glEnd();
//     glDeleteTextures(1, &quitButtonTexID); // Clean up texture

//     glDisable(GL_TEXTURE_2D);

//     // Play background music
//     sf::Music music;
//     if (music.openFromFile("John_Harrison_with_the_Wichita_State_University_Chamber_Players_-_01_-_Spring_Mvt_1_Allegro.mp3"))
//     {
//         music.setLoop(true);
//         music.play();
//     }
// }