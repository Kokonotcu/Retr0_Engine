#version 450

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in uint inUV; // two halfs in 16 bits each
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uint inColor; // four UNORM8

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;

vec2 decodeUV(uint pack)   { return unpackHalf2x16(pack); }
vec4 decodeColor(uint pack){ return unpackUnorm4x8(pack); }

//push constants block
layout( push_constant ) uniform constants
{	
	mat4 render_matrix;
	mat4 model;
} PushConstants;

void main() 
{
	vec3 norm  = inNormal;
	vec2 uv    = decodeUV(inUV);
	vec4 color = decodeColor(inColor);
	
	//output data
	gl_Position = PushConstants.render_matrix * vec4(inPosition, 1.0f);
	outColor = color;
	outUV = uv;
}