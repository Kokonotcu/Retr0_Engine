#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=Scale, z=EdgeSharpness
} PC;

// Random 2D -> 2D
vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

void main() {
    float time = PC.params.x;
    float scale = 8.0;
    
    // --- WATERFALL LOGIC ---
    vec3 pos = inWorldPos;
    
    // 1. Flow: Move coordinate system UP (so texture moves DOWN)
    pos.y += time * 2.0; 
    
    // 2. Stretch: Compress Y coordinates to make pattern look elongated
    // The smaller the number, the longer the cells look.
    pos.y *= 0.3; 
    
    // Map to grid
    vec2 st = (pos.xy + pos.z * 0.1) * scale;
    // -----------------------

    vec2 i_st = floor(st);
    vec2 f_st = fract(st);

    float m_dist = 1.0;  // Minimum distance

    // Voronoi Loop (Check 3x3 neighbors)
    for (int y= -1; y <= 1; y++) {
        for (int x= -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x),float(y));
            
            // Random point location
            vec2 point = random2(i_st + neighbor);

            // Animate the internal points slightly for "organic" bubbling
            point = 0.5 + 0.5*sin(time * 2.0 + 6.2831*point);
            
            vec2 diff = neighbor + point - f_st;
            float dist = length(diff);

            if( dist < m_dist ) {
                m_dist = dist;
            }
        }
    }

    // --- COLORING ---
    // Invert distance: Center(1.0) -> Edge(0.0)
    float intensity = 1.0 - m_dist; 
    
    // Sharpen the veins
    float sharpness =  5.0;
    intensity = pow(intensity, sharpness);

    vec3 veinColor = vec3(0.0, 1.0, 0.8); // Glowing Cyan
    vec3 cellColor = vec3(0.0, 0.1, 0.2); // Dark Blue background
    
    // Add a "highlight" wave that moves down independently
    float highlight = sin(inWorldPos.y * 2.0 + time * 5.0);
    veinColor += vec3(0.5) * smoothstep(0.8, 1.0, highlight);

    vec3 finalColor = mix(cellColor, veinColor, intensity);

    outFragColor = vec4(finalColor, 1.0);
}