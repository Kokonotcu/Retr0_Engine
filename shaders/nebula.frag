#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=Density, z=ColorShift
} PC;

// Rotation matrix
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// 3D FBM Noise (Fractal Brownian Motion)
float hash(vec3 p) {
    p  = fract(p * 0.3183099 + .1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix( hash(i + vec3(0,0,0)), hash(i + vec3(1,0,0)),f.x),
                   mix( hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)),f.x),f.y),
               mix(mix( hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)),f.x),
                   mix( hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)),f.x),f.y),f.z);
}

float map(vec3 p) {
    vec3 q = p - vec3(0.0, 0.1, 1.0);
    q.xy *= rot(PC.params.x * 0.5); // Rotate galaxy
    q.xz *= rot(PC.params.x * 0.2);
    
    float f = 0.0;
    float s = 0.5;
    for(int i=0; i<4; i++) {
        f += s * noise(q * 3.0); 
        q *= 2.0;
        s *= 0.5;
    }
    return clamp(f - 0.3, 0.0, 1.0); // Density threshold
}

void main() {
    // Raymarch setup
    vec3 ro = inWorldPos * 1.2 + vec3(0,0,2); // Fake camera origin
    vec3 rd = normalize(inWorldPos - vec3(0,0,5)); // View direction
    
    vec3 p = ro;
    vec3 col = vec3(0.0);
    float dens = 0.0;
    
    // Accumulate fog
    for (int i=0; i<15; i++) {
        float d = map(p);
        
        // Coloring based on density
        vec3 fogCol = mix(vec3(0.1, 0.2, 0.8), vec3(0.8, 0.1, 0.4), d * PC.params.z);
        
        col += fogCol * d * 0.1 * (PC.params.y > 0.0 ? PC.params.y : 1.0);
        dens += d * 0.05;
        p += rd * 0.1; // Step forward
        
        if (dens > 1.0) break;
    }
    
    // Add stars (high frequency noise)
    float stars = pow(hash(inWorldPos * 50.0), 20.0);
    col += vec3(stars);

    outFragColor = vec4(col, 1.0);
}