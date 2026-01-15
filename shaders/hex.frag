#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=HexSize, z=ImpactStrength
} PC;

// Hexagon distance function
float hexDist(vec2 p) {
    p = abs(p);
    float c = dot(p, normalize(vec2(1, 1.73)));
    c = max(c, p.x);
    return c;
}

vec4 getHex(vec2 p) {
    vec2 r = vec2(1, 1.73);
    vec2 h = r * 0.5;
    vec2 a = mod(p, r) - h;
    vec2 b = mod(p - h, r) - h;
    vec2 gv = dot(a, a) < dot(b, b) ? a : b;
    float x = atan(gv.x, gv.y);
    float y = 0.5 - hexDist(gv);
    vec2 id = p - gv;
    return vec4(x, y, id.x, id.y);
}

void main() {
    float size =  5.0;
    vec2 uv = inWorldPos.xz * size + inWorldPos.y * 0.5; // Map to cylinder-ish UV
    
    // Animate hex grid scrolling
    uv.y += PC.params.x * 0.5;
    
    vec4 h = getHex(uv);
    
    // Hex Edges
    float dist = smoothstep(0.02, 0.05, h.y);
    
    // Pulse effect
    float pulse = sin(h.z * 10.0 + h.w * 5.0 - PC.params.x * 5.0);
    vec3 hexColor = vec3(0.0, 0.5, 1.0) + pulse * 0.5; // Blue base + Pulse
    
    // "Impact" Ring
    float impactDist = length(inWorldPos.xy); // Distance from center
    float impactWave = sin(impactDist * 3.0 - PC.params.x * 10.0);
    float impactStr = smoothstep(0.8, 1.0, impactWave);
    
    // Combine
    vec3 finalColor = mix(vec3(0.0), hexColor, dist); // Grid
    finalColor += vec3(1.0, 0.8, 0.2) * impactStr ; // Orange Impact hit
    
    // Fresnel Rim (always needed for shields)
    float rim = 1.0 - abs(dot(inNormal, vec3(0,0,1)));
    finalColor += vec3(0.0, 0.5, 1.0) * pow(rim, 3.0);

    outFragColor = vec4(finalColor, 0.8 * dist + rim + impactStr); // Alpha transparency
}