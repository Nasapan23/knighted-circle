#version 330 core
layout(location = 0) in vec2 aPos;

uniform vec2 uOffset;
uniform float uScale;
uniform mat4 uProjection; // Projection matrix uniform
uniform mat4 uModel;      // Model matrix uniform

void main()
{
    // For circles (player, enemies, arrow)
    vec2 pos = aPos * uScale + uOffset;
    
    // Apply model and projection
    gl_Position = uProjection * uModel * vec4(pos, 0.0, 1.0);
}
