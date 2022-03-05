#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;	
layout(location = 3) in vec3 color;

uniform mat4 view_proj;
uniform mat4 model;
uniform mat3 normal_transform;

out VertexData {
	vec3 position, normal, color;
	vec2 uv;
} vs_out;

void main(){
	vec4 pos = model * vec4(position, 1.0);
	vs_out.position = pos.xyz;
	vs_out.uv = uv;
	vs_out.normal = normal_transform * normal;
	vs_out.color = color;
	gl_Position = view_proj * pos;
}
