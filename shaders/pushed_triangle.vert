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

vec2 decodeUV(uint packed)   { return unpackHalf2x16(packed); }
vec4 decodeColor(uint packed){ return unpackUnorm4x8(packed); }

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
	
	vec3 norm  = normalize(v.normal);
	vec2 uv    = decodeUV(v.uv);
	vec4 color = decodeColor(v.color);
	
	//output data
	gl_Position = PushConstants.render_matrix *vec4(v.position, 1.0f);
	outColor = color;
	outUV = uv;
}