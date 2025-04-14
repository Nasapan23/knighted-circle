#ifndef SHADER_H
#define SHADER_H

#include <string>
#include "dependente/glew/glew.h"

class Shader {
public:
    GLuint ID;

    Shader(const char* vertexPath, const char* fragmentPath);
    void use();
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, const float* value) const;
};

#endif
