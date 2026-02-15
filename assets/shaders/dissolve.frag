#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time/Cutoff, y=EdgeWidth, z=Intensity
} PC;

// A simple pseudo-random hash function
float hash(vec3 p) {
    p  = fract(p * 0.3183099 + .1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// 3D Noise function
float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix( hash(i + vec3(0,0,0)), hash(i + vec3(1,0,0)),f.x),
                   mix( hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)),f.x),f.y),
               mix(mix( hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)),f.x),
                   mix( hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)),f.x),f.y),f.z);
}

void main() {
    // 1. Generate Noise Pattern based on World Position
    // Scale the position so the noise isn't too tiny
    float n = noise(inWorldPos * 5.0); 
    
    // 2. Animate the cutoff threshold
    // Sine wave oscillates between -0.2 and 1.2 to fully show/hide object
    float cutOff = sin(PC.params.x) * 0.6 + 0.5;
    
    // 3. Discard logic
    if (n < cutOff) {
        discard; // Physically remove pixel
    }
    
    // 4. Glow Edge Logic
    // If the noise value is VERY close to the cutoff, paint it fire color
    float edgeWidth = PC.params.y > 0.0 ? PC.params.y : 0.05;
    
    vec3 baseColor = vec3(0.1, 0.1, 0.1); // Dark Gray Mesh
    vec3 fireColor = vec3(1.0, 0.3, 0.0); // Orange Glow
    
    // Step function to create the band
    float edge = step(cutOff + edgeWidth, n);
    
    // Mix between Fire and Base based on edge step
    outFragColor = vec4(mix(fireColor, baseColor, edge), 1.0);
}