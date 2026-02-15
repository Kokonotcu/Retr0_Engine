#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;

struct Vertex {
    vec3  position;
	uint  uv; // two halfs in 16 bits each
    vec3  normal;
    uint  color; // four UNORM8
};

vec2 decodeUV(uint pack)   { return unpackHalf2x16(pack); }
vec4 decodeColor(uint pack){ return unpackUnorm4x8(pack); }

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{	
	mat4 render_matrix;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{	
	//load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	
	vec3 norm  = v.normal;
	vec2 uv    = decodeUV(v.uv);
	vec4 color = decodeColor(v.color);
	
	//output data
	gl_Position = PushConstants.render_matrix *vec4(v.position, 1.0f);
	outColor = color;
	outUV = uv;
}