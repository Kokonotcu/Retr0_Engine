#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in uint inUV;       
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uint inColor;    

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outWorldPos; // Crucial for effects

vec2 decodeUV(uint pack)    { return unpackHalf2x16(pack); }
vec4 decodeColor(uint pack) { return unpackUnorm4x8(pack); }

layout(push_constant) uniform constants
{
    mat4 render_matrix; // ViewProj
    mat4 model;         // Object -> World
} PushConstants;

void main() 
{
    outUV = decodeUV(inUV);
    outColor = decodeColor(inColor);
    
    // Rotate normal to world space (simplified, assumes uniform scale)
    outNormal = normalize(mat3(PushConstants.model) * inNormal);
    
    // Calculate World Position for lighting/effects
    vec4 worldPos = PushConstants.model * vec4(inPosition, 1.0);
    outWorldPos = worldPos.xyz;
    
    // Final Project Position
    gl_Position = PushConstants.render_matrix * worldPos;
}