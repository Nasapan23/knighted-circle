#version 330 core
layout(location = 0) in vec2 aPos;

uniform vec2 uOffset;
uniform float uScale;
uniform mat4 uProjection; // Projection matrix uniform

void main()
{
    vec2 pos = aPos * uScale + uOffset;
    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
}
