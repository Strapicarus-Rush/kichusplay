#include <iostream>
// #include <ft2build.h>
#include "freetype.h"
#include FT_FREETYPE_H

#include <GL/gl.h>  // Include OpenGL headers
#include <GL/glext.h>
#include <GL/glcorearb.h>
#include <GLES3/gl32.h>

GLuint shader;      // Declare shader variable
GLuint VAO;         // Declare VAO variable

// Initialize FreeType library and load font
FT_Library ft;
FT_Face face;

void initFreeType(const char* fontPath) {
    
    // Initialize FreeType
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Failed to initialize FreeType" << std::endl;
        // Handle error
    }

    // Load font face
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        std::cerr << "Failed to load font" << std::endl;
        // Handle error
    }

    // Set font size
    FT_Set_Pixel_Sizes(face, 0, 48); // Set font size (width, height)
}

void renderText(FT_Face face, const char* text, float x, float y, float scale, float r, float g, float b) {
    // Activate corresponding render state
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), r, g, b); // Use the passed color components
    glBindVertexArray(VAO);  // Bind VAO

    // Iterate through all characters
    const char* p;
    for (p = text; *p; p++) {
        if (FT_Load_Char(face, *p, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph" << std::endl;
            continue;
        }

        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (face->glyph->advance.x >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }

    glBindVertexArray(0); // Unbind VAO
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
}