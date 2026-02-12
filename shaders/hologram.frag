#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=Speed, z=Alpha
} PC;

void main() {
    // Fresnel (Rim light)
    vec3 viewDir = normalize(vec3(0, 0, 1)); // Simplified
    float fresnel = pow(1.0 - abs(dot(inNormal, viewDir)), 2.0);
    
    // Moving Scanline
    float scan = sin(inWorldPos.y * 10.0 - PC.params.x * PC.params.y);
    scan = smoothstep(0.8, 1.0, scan);
    
    vec3 holoColor = vec3(0.2, 0.4, 1.0); // Blue
    float alpha = fresnel + scan;
    
    // Combine
    outFragColor = vec4(holoColor * 1.5, alpha * PC.params.z);
}