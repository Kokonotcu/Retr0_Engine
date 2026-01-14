#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in uint inUV;      // two halfs in 16 bits each
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uint inColor;   // four UNORM8

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;

vec2 decodeUV(uint pack)    { return unpackHalf2x16(pack); }
vec4 decodeColor(uint pack) { return unpackUnorm4x8(pack); }

layout(push_constant) uniform constants
{
    mat4 render_matrix; // usually viewProj
    mat4 model;         // object -> world
} PushConstants;

void main()
{
    vec2 uv    = decodeUV(inUV);
    vec4 color = decodeColor(inColor);

    // Positions
    vec4 worldPos = PushConstants.model * vec4(inPosition, 1.0);
    gl_Position = PushConstants.render_matrix * vec4(inPosition, 1.0);

    // Proper normal transform (handles rotation + non-uniform scale)
    mat3 normalMat = transpose(inverse(mat3(PushConstants.model)));
    vec3 N = normalize(normalMat * inNormal);

    // Point light in world space
    vec3 lightPos = vec3(45.0, 45.0, 45.0);
    vec3 L = normalize(lightPos - worldPos.xyz);

    float diff = max(dot(N, L), 0.0);

    // Apply Lambert diffuse to your vertex color (keep alpha)
    outColor = vec4(color.rgb * diff * 2.0f, color.a);
    outUV = uv;
}
