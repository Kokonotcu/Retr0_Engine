#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in uint inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uint inColor;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outWorldPos;

vec2 decodeUV(uint pack)    { return unpackHalf2x16(pack); }
vec4 decodeColor(uint pack) { return unpackUnorm4x8(pack); }

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=Strength, z=Frequency
} PC;

// Random noise function
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

void main() {
    outUV = decodeUV(inUV);
    outColor = decodeColor(inColor);
    outNormal = normalize(mat3(PC.model) * inNormal);

    // --- GLITCH LOGIC ---
    vec3 pos = inPosition;
    float time = PC.params.x;
    
    // Create a "strip" of glitch that moves up/down
    // If the Y position matches the Time signal, snap the vertex
    float noise = random(vec2(pos.y * 0.5, time));
    float glitchStrip = sin(pos.y * 5.0 + time * 10.0);
    
    // If we are in a "glitch zone", offset X position violently
    if (glitchStrip > 0.8 && noise > 0.5) {
        float strength = PC.params.y > 0.0 ? PC.params.y : 0.5;
        pos.x += sin(time * 50.0) * strength;
        pos.z += cos(time * 40.0) * strength * 0.5;
    }
    // --------------------

    vec4 worldPos = PC.model * vec4(pos, 1.0);
    outWorldPos = worldPos.xyz;
    gl_Position = PC.render_matrix * worldPos;
}