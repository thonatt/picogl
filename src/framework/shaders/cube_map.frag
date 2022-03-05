#version 460 

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 color;

uniform samplerCube cubemap;
uniform mat3 camera_ray_derivatives;

void main() 
{	
    const vec3 ray_dir = uv.x * camera_ray_derivatives[0] + uv.y * camera_ray_derivatives[1] + camera_ray_derivatives[2];
	const vec3 dir_n = normalize(ray_dir);
	color = texture(cubemap, dir_n);
	//color = vec4(0.5 + 0.5 *dir_n, 1);
}