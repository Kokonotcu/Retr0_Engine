#version 450

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=Bands (3.0-6.0), z=Outline Width
} PC;

void main() {
    // 1. Animated Light Direction (Rotates around Y axis)
    float time = PC.params.x;
    vec3 lightDir = normalize(vec3(sin(time), 0.5, cos(time)));

    // 2. Calculate Intensity (Standard diffuse lighting)
    vec3 normal = normalize(inNormal);
    float intensity = dot(normal, lightDir);
    
    // 3. Toon Logic: Quantize intensity into "steps"
    // Example: If bands=3, intensity becomes 0.0, 0.33, 0.66, or 1.0
    float bands = PC.params.y > 0.0 ? PC.params.y : 4.0;
    intensity = ceil(intensity * bands) / bands;
    intensity = max(intensity, 0.2); // Minimum ambient light

    // 4. Outline Logic (Rim Light check)
    // If the normal is perpendicular to view, it's an edge -> paint black
    vec3 viewDir = normalize(vec3(0, 0, 1)); // Simplified view
    float edge = dot(viewDir, normal);
    float outlineWidth = PC.params.z > 0.0 ? PC.params.z : 0.3;
    
    vec3 surfaceColor = vec3(0.0, 0.5, 1.0); // Base Blue
    
    if (edge < outlineWidth && edge > 0.0) {
        outFragColor = vec4(0.0, 0.0, 0.0, 1.0); // Black Outline
    } else {
        outFragColor = vec4(surfaceColor * intensity *2.0, 1.0);
    }
}