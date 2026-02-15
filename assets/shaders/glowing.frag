#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=FoldScale, z=Glow
} PC;

// Rotation helper
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Distance estimator for the fractal
float map(vec3 p) {
    float scale =  1.5;
    float time = PC.params.x;
    
    // Animate the space itself
    p.xz *= rot(time * 0.2);
    p.xy *= rot(time * 0.1);
    
    float d = 1000.0;
    float s = 1.0;
    
    // Fractal Iteration Loop
    for (int i = 0; i < 6; i++) {
        // Fold space: abs(p) mirrors the coordinate system
        p = abs(p) - 0.5; 
        
        // Rotate each iteration
        p.xy *= rot(0.5);
        p.xz *= rot(0.3);
        
        // Scale down
        p *= scale;
        s *= scale;
        
        // Accumulate distance (Box shape)
        d = min(d, (length(max(abs(p) - 0.2, 0.0))) / s);
    }
    
    return d;
}

void main() {
    // We are "raymarching" locally around the surface
    vec3 viewDir = normalize(inWorldPos - vec3(0,0,5)); // Fake camera pos relative to object
    vec3 p = inWorldPos * 2.0; // Local coordinate space
    
    float totalD = 0.0;
    float minDist = 100.0;
    
    // Raymarch loop (simplified for surface shader)
    // We sample the fractal distance field nearby
    float d = map(p);
    
    // Color based on how close we are to the mathematical fractal surface
    float glow = 0.005 / (d + 0.0001); // Inverse square law for glow
    
    // Palette
    vec3 col = vec3(0.1, 0.4, 0.8) * glow ;
    col += vec3(1.0, 0.5, 0.2) * glow * glow * 0.5; // Hot core
    
    // Mask with original mesh normal to keep some solidity
    float rim = 1.0 - abs(dot(inNormal, vec3(0,0,1)));
    
    outFragColor = vec4(col + rim * 0.2, 1.0);
}