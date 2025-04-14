#ifndef FONT_H
#define FONT_H

#include <map>
#include <string>
#include "dependente/glew/glew.h"
#include "dependente/glm/glm.hpp"

// Character structure matching the LearnOpenGL tutorial
struct Character {
    GLuint TextureID;  // ID handle of the glyph texture
    glm::ivec2 Size;   // Size of glyph
    glm::ivec2 Bearing;// Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};

class Font {
public:
    // Constructor/Destructor
    Font();
    ~Font();

    // Initialize font with given font file and size
    bool init(const char* fontPath, int fontSize);
    
    // Render text at the specified position with given scale and color
    void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color);
    
    // Set the shader to use for rendering
    void setShader(GLuint shaderProgram);

private:
    // Map of ASCII characters to their corresponding Character struct
    std::map<char, Character> Characters;
    
    // VAO and VBO for text rendering
    GLuint VAO, VBO;
    
    // Shader program ID
    GLuint shader;
};

#endif 