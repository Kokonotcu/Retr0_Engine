#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time
} PC;

void main() {
    float r = sin(inWorldPos.x * 2.0 + PC.params.x);
    float g = sin(inWorldPos.y * 2.0 + PC.params.x * 1.5);
    float b = sin(inWorldPos.z * 2.0 + PC.params.x * 0.5);
    
    outFragColor = vec4(r*0.5+0.5, g*0.5+0.5, b*0.5+0.5, 1.0);
}