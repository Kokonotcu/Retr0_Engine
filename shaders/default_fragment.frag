#version 450

// Inputs from Vertex Shader (must match layout locations)
layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

void main() {

    // 1. Re-normalize inputs (Interpolation can denormalize vectors)
    vec3 N = normalize(inNormal);

    // 2. Define Light (Hardcoded or use a Uniform)
    vec3 lightPos = vec3(-1.0, 1.0, 2.0); 
    vec3 L = normalize(lightPos - inWorldPos);

    // 3. Calculate Diffuse Lighting
    float diff = max(dot(N, L), 0.0);
    
    // 4. Optional: Ambient Light (Prevents pitch black shadows)
    vec3 ambient = vec3(0.01, 0.01, 0.01);

    // 5. Combine results
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 finalLight = (lightColor * diff) + ambient;

    // Apply lighting to the vertex color
    outColor = vec4(finalLight* inColor.rgb, inColor.a); //* inColor.rgb
}