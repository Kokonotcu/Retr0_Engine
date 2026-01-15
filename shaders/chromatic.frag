#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=OffsetAmount, z=Frequency
} PC;

void main() {
    // View direction
    vec3 viewDir = normalize(vec3(0,0,10) - inWorldPos);
    float NdotV = dot(normalize(inNormal), viewDir);
    
    // Calculated "Refraction" offset based on angle
    // Edges (NdotV close to 0) get more distortion
    float offset = (1.0 - NdotV) * ( 0.1);
    
    // Animate the offset slightly
    offset *= (1.0 + sin(PC.params.x * 10.0) * 0.2);

    // Fake environment mapping (lighting)
    // We sample the "light" at 3 slightly different normal angles
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    
    vec3 rNormal = normalize(inNormal + vec3(offset, 0, 0));
    vec3 gNormal = normalize(inNormal);
    vec3 bNormal = normalize(inNormal - vec3(offset, 0, 0));
    
    float r = dot(rNormal, lightDir);
    float g = dot(gNormal, lightDir);
    float b = dot(bNormal, lightDir);
    
    // Contrast boost
    vec3 col = vec3(r, g, b);
    col = pow(col, vec3(3.0)); // Sharpen highlights
    
    outFragColor = vec4(col, 1.0);
}