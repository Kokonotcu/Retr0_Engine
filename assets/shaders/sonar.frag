#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=PulseWidth, z=GridDensity
} PC;

void main() {
    // 1. The Pulse (Spherical distance from origin)
    float dist = length(inWorldPos);
    float time = mod(PC.params.x * 5.0, 15.0); // Loop the scan
    
    float pulseWidth =  1.0;
    float wave = 1.0 - smoothstep(0.0, pulseWidth, abs(dist - time));
    
    // 2. The Grid (Triplanar projection approximation)
    float gridDensity = 5.0;
    vec3 gridPos = fract(inWorldPos * gridDensity);
    float gridLine = step(0.95, gridPos.x) + step(0.95, gridPos.y) + step(0.95, gridPos.z);
    gridLine = clamp(gridLine, 0.0, 1.0);
    
    // 3. Colors
    vec3 baseColor = vec3(0.01, 0.01, 0.05); // Almost black
    vec3 scanColor = vec3(0.0, 1.0, 0.5);    // Matrix Green
    vec3 gridColor = vec3(0.0, 0.3, 0.1);
    
    // Composition
    vec3 finalColor = baseColor;
    
    // Add glowing grid everywhere weakly
    finalColor += gridColor * gridLine * 0.2;
    
    // Add strong pulse
    finalColor += scanColor * wave * (1.0 + gridLine * 5.0); // Grid glows super bright in the wave
    
    outFragColor = vec4(finalColor, 1.0);
}