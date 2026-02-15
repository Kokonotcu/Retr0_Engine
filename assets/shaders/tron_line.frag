#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=Density(1.0-10.0), z=Thickness
} PC;

void main() {
    // Grid math
    float scale = PC.params.y > 0.1 ? PC.params.y : 5.0;
    vec2 grid = fract(inWorldPos.xz * scale);
    
    float thickness = PC.params.z > 0.0 ? PC.params.z : 0.05;
    float lines = step(1.0 - thickness, grid.x) + step(1.0 - thickness, grid.y);
    
    vec3 glow = vec3(0.0, 1.0, 0.8) * 2.0; // Cyan Glow
    vec3 base = vec3(0.1, 0.1, 0.15);      // Dark base
    
    outFragColor = vec4(mix(base, glow, lines), 1.0);
}