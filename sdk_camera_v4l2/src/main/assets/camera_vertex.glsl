#version 310 es
uniform mat4 vMatrix;
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vTexCoord;
out vec2 texCoord;

void main() {
   texCoord = vTexCoord;
   gl_Position = vMatrix*vPosition;
}