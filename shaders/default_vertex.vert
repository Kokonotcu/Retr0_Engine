#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in uint inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uint inColor;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;   
layout (location = 3) out vec3 outWorldPos; 

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} cameraData;

layout(push_constant) uniform constants {
    mat4 transform;       
} PushConstants;

vec2 decodeUV(uint pack)    { return unpackHalf2x16(pack); }
vec4 decodeColor(uint pack) { return unpackUnorm4x8(pack); }

void main() 
{
    outUV = decodeUV(inUV);
    outColor = decodeColor(inColor);

    vec4 worldPos = PushConstants.transform * vec4(inPosition, 1.0);
    outWorldPos = worldPos.xyz;
    
    outNormal = normalize(mat3(PushConstants.normalMatrix) * inNormal);

    // 3. Final Position for the GPU
    gl_Position = cameraData.viewproj * worldPos;
}