#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=CrustLevel, z=Heat
} PC;

// Simple random
float random(in vec2 _st) {
    return fract(sin(dot(_st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

// 2D Noise
float noise(in vec2 _st) {
    vec2 i = floor(_st);
    vec2 f = fract(_st);
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a)* u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 st) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 5; i++) {
        v += a * noise(st);
        st = st * 2.0;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec2 st = inWorldPos.xy * 2.0 + inWorldPos.z; // Wrap coords
    float time = PC.params.x;
    
    // Warping the texture coordinates for "flow"
    vec2 q = vec2(0.);
    q.x = fbm(st + 0.00 * time);
    q.y = fbm(st + vec2(1.0));

    vec2 r = vec2(0.);
    r.x = fbm(st + 1.0 * q + vec2(1.7, 9.2) + 0.15 * time);
    r.y = fbm(st + 1.0 * q + vec2(8.3, 2.8) + 0.126 * time);

    float f = fbm(st + r);

    // Color Palette (Black Rock -> Red -> Yellow -> White Hot)
    vec3 rock = vec3(0.1, 0.05, 0.0);
    vec3 lava = vec3(0.8, 0.1, 0.0);
    vec3 hot = vec3(1.0, 0.8, 0.2);
    
    vec3 color = mix(rock, lava, smoothstep(0.2, 0.6, f));
    color = mix(color, hot, smoothstep(0.6, 1.0, f));
    
    // Add "Heat" glow
    float heat = 1.5;
    color *= heat;

    outFragColor = vec4(color, 1.0);
}