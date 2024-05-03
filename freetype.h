#ifndef FREETYPE_H
#define FREETYPE_H

#include <ft2build.h>
#include FT_FREETYPE_H



// FT_Face face;
extern FT_Face face; // Declare as extern
// Function declarations
void initFreeType(const char* fontPath);
void renderText(FT_Face face, const char* text, float x, float y, float scale, float r, float g, float b);

#endif // FREETYPE_H