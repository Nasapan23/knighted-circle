#include "Font.h"
#include "dependente/glm/gtc/matrix_transform.hpp"
#include "dependente/glm/gtc/type_ptr.hpp"
#include <iostream>
#include <fstream>
#include <vector>

// Force recompile - 1

// Include STB libraries
#define STB_TRUETYPE_IMPLEMENTATION
#include "dependente/stb-master/stb_truetype.h"

// Only define STB_IMAGE_IMPLEMENTATION if not already defined elsewhere
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "dependente/stb-master/stb_image.h"
#endif

Font::Font() : VAO(0), VBO(0), shader(0) {
}

Font::~Font() {
    // Clean up textures
    for (auto& ch : Characters) {
        glDeleteTextures(1, &ch.second.TextureID);
    }
    
    // Clean up buffers
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
    }
}

bool Font::init(const char* fontPath, int fontSize) {
    // Clear existing character data if any
    for (auto& ch : Characters) {
        glDeleteTextures(1, &ch.second.TextureID);
    }
    Characters.clear();

    // Read font file
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open font file: " << fontPath << std::endl;
        return false;
    }
    
    // Get file size and read file data
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<unsigned char> fontBuffer(size);
    if (!file.read(reinterpret_cast<char*>(fontBuffer.data()), size)) {
        std::cerr << "Failed to read font file data" << std::endl;
        return false;
    }
    
    // Initialize stb_truetype
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontBuffer.data(), 0)) {
        std::cerr << "Failed to initialize stb_truetype" << std::endl;
        return false;
    }
    
    // Calculate font scaling factor
    float scale = stbtt_ScaleForPixelHeight(&font, static_cast<float>(fontSize));
    
    // Store OpenGL state
    GLint alignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction
    
    // Load first 128 ASCII characters
    for (unsigned char c = 32; c < 128; c++) {
        // Get glyph bitmap and metrics
        int width, height, xoff, yoff;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(
            &font, 0, scale, c, &width, &height, &xoff, &yoff
        );
        
        if (!bitmap) {
            std::cerr << "Failed to get bitmap for character: " << c << std::endl;
            continue;
        }
        
        // Create texture for character
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            width, height,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            bitmap
        );
        
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Get character metrics
        int advance, lsb, x0, y0, x1, y1;
        stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&font, c, scale, scale, &x0, &y0, &x1, &y1);
        
        // Store character data - follow LearnOpenGL exactly
        Character character = {
            texture,
            glm::ivec2(width, height),
            glm::ivec2(xoff, -yoff), // xoff for horizontal bearing, adjust yoff for OpenGL coords
            static_cast<unsigned int>(advance) // Store the UNSCALED advance value
        };
        
        Characters.insert(std::pair<char, Character>(c, character));
        
        // Free the bitmap
        stbtt_FreeBitmap(bitmap, nullptr);
    }
    
    // Restore OpenGL state
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Configure VAO/VBO for text quads
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Reserve memory - we'll be updating with new data when rendering each character
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return !Characters.empty();
}

void Font::setShader(GLuint shaderProgram) {
    shader = shaderProgram;
}

void Font::renderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    if (!shader || Characters.empty()) {
        return;
    }
    
    // Enable blending for text rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Activate corresponding render state
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    
    // Iterate through all characters
    float x_pos = x;
    for (char c : text) {
        if (Characters.find(c) == Characters.end()) {
            continue;
        }
        
        Character ch = Characters[c];
        
        // Pre-calculate scaled values
        float scaled_bearing_x = ch.Bearing.x * scale;
        // More precise advance: Scale first, then divide by 64.0f
        float scaled_advance = (ch.Advance * scale) / 64.0f; 
        float scaled_width = ch.Size.x * scale;
        float scaled_height = ch.Size.y * scale;

        // Calculate positions EXACTLY as in the LearnOpenGL tutorial
        float xpos = x_pos + scaled_bearing_x; // Use pre-calculated scaled bearing
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        
        float w = scaled_width; // Use pre-calculated scaled width
        float h = scaled_height; // Use pre-calculated scaled height
        
        // Update VBO for each character - match EXACTLY from tutorial
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f }
        };
        
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // Render quad (only if character has dimensions)
        if (w > 0 && h > 0) {
             glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        
        // Now advance cursor for next glyph
        x_pos += scaled_advance; // Use the more precise scaled advance
    }
    
    // Restore state
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
} 