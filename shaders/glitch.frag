#version 450
layout (location = 0) in vec4 inColor;
layout (location = 3) in vec3 inWorldPos;
layout (location = 0) out vec4 outFragColor;

void main() {
    // Digital Green/Red matrix style
    outFragColor = vec4(0.0, 1.0, 0.2, 1.0); 
}