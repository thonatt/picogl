#version 460

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

layout(location = 0) out vec4 color;
layout(location = 1) out ivec3 object_instance_id;

in VertexData {
	vec3 position, normal, color;
	vec2 uv;
	flat int global_instance_id;
} frag_in;

uniform vec3 light_pos;
uniform vec3 camera_pos;
uniform sampler2D sampler;

// https://www.shadertoy.com/view/ttc3zr
uvec3 murmurHash31(uint src) {
    const uint M = 0x5bd1e995u;
    uvec3 h = uvec3(1190494759u, 2147483647u, 3559788179u);
    src *= M; src ^= src>>24u; src *= M;
    h *= M; h ^= src;
    h ^= h>>13u; h *= M; h ^= h>>15u;
    return h;
}

vec3 hash31(uint src) {
    uvec3 h = murmurHash31(src);
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

vec3 phong(vec3 position, vec3 normal)
{
	const float kd = 0.3;
	const float ks = 0.2;
	const vec3 meshColor = vec3(0.7);

	vec3 L = normalize(light_pos - position);
	vec3 N = normalize(normal);
	vec3 V = normalize(camera_pos - position);
	vec3 R = reflect(-L, N);
	float diffuse = max(0.0, dot(L, N));
	float specular = max(0.0, dot(R, V));
	
	return (1.0 - kd - kd)*meshColor + (kd*diffuse + ks*specular)*vec3(1.0);
}

void main()
{
	InstanceData instance = instance_data[frag_in.global_instance_id];
	
	object_instance_id = 1 + ivec3(instance.object_id, instance.instance_id, frag_in.global_instance_id);
	
	switch(instance.rendering_mode){
		default:
		{
			color = vec4(phong(frag_in.position, frag_in.normal), 1);
			break;
		}
		case 1:
			color = vec4(frag_in.uv, 0, 1);
			break;
		case 2:
			color = vec4(0.5*normalize(frag_in.normal) + vec3(0.5), 1);
			break;
		case 3:
			color = vec4(fract(frag_in.position), 1);
			break;
		case 4:
			color = vec4(hash31(uint(frag_in.global_instance_id)), 1);
			break;
		case 5:
			color = texture(sampler, frag_in.uv);
			break;
	}
}