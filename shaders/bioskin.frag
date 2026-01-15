#version 450
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants {
    mat4 render_matrix;
    mat4 model;
    vec4 params; // x=Time, y=Scale, z=Jitter
} PC;

// Random 2D -> 2D
vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

void main() {
    float scale =  5.0;
    float jitter =  1.0;
    
    // Map World Position to 2D grid (using X and Y + Z for some 3D feel)
    vec2 st = (inWorldPos.xy + inWorldPos.zz * 0.5) * scale;
    
    // Tile the space
    vec2 i_st = floor(st);
    vec2 f_st = fract(st);

    float m_dist = 1.0;  // Minimum distance
    vec2 m_point;        // Minimum point

    // Voronoi Loop: Check neighbor grid cells
    for (int y= -1; y <= 1; y++) {
        for (int x= -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x),float(y));
            
            // Random point in that neighbor cell
            vec2 point = random2(i_st + neighbor);
            
            // Animate the point
            point = 0.5 + 0.5*sin(PC.params.x + 6.2831*point);
            
            // Vector to that point
            vec2 diff = neighbor + point - f_st;
            
            // Distance
            float dist = length(diff);

            // Keep the closest distance
            if( dist < m_dist ) {
                m_dist = dist;
            }
        }
    }

    // Visualizing the distance field
    // Center of cell = dark, Edge = bright
    vec3 color = vec3(0.0, 0.5, 0.8); // Biological Blue
    color += m_dist * m_dist;         // Add glow based on distance
    
    // Add "Cell Walls"
    float wall = step(0.9, m_dist); // Sharp edges
    color = mix(color, vec3(1.0), wall * 0.5); // White veins

    outFragColor = vec4(color, 1.0);
}