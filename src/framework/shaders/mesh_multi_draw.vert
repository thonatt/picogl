#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;	
layout(location = 3) in vec3 color;

struct InstanceData
{
	mat4 object_to_world;
	mat4 normal_to_world;
	uint object_id;
	uint instance_id;
	uint rendering_mode;
	int pad;
};

layout(std430, binding = 0) readonly buffer InstanceBuffer
{
	InstanceData instance_data[];
};

layout(std430, binding = 1) readonly buffer InstanceOffset
{
	int instance_offsets[];
};

uniform mat4 view_proj;

out VertexData {
	vec3 position, normal, color;
	vec2 uv;
	flat int global_instance_id;
} vs_out;

void main(){
	int instance_offset = instance_offsets[gl_DrawID];
	int global_instance_id = gl_InstanceID + instance_offset;
	InstanceData instance = instance_data[global_instance_id];
	
	vec4 pos = mat4(instance.object_to_world) * vec4(position, 1.0);
	vs_out.position = pos.xyz;
	vs_out.uv = uv;
	vs_out.normal = mat3(instance.normal_to_world) * normal;
	vs_out.color = color;
	vs_out.global_instance_id = global_instance_id;
	gl_Position = view_proj * pos;
}
