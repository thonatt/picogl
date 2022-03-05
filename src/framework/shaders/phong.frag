#version 460

layout(location = 0) out vec4 color;

in VertexData {
	vec3 position, normal, color;
	vec2 uv;
} frag_in;

uniform vec3 light_pos;
uniform vec3 camera_pos;

void main()
{
	const float kd = 0.3;
	const float ks = 0.2;
	const vec3 meshColor = vec3(0.7);

	vec3 L = normalize(light_pos - frag_in.position);
	vec3 N = normalize(frag_in.normal);
	vec3 V = normalize(camera_pos - frag_in.position);
	vec3 R = reflect(-L,N);
	float diffuse = max(0.0, dot(L,N));
	float specular = max(0.0, dot(R,V));

	color = vec4( (1.0 - kd - kd)*meshColor + (kd*diffuse + ks*specular)*vec3(1.0) , 1.0);
}