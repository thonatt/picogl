#version 460 

uniform mat3 screen_to_uv;
uniform float lod = -1.0;

layout(binding = 0) uniform sampler2D sampler;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

void main() 
{
    vec2 uv = vec2(screen_to_uv * vec3(in_uv, 1));
	if(lod >= 0.0)
		out_color = textureLod(sampler, uv, lod);
	else
		out_color = texture(sampler, uv);
		
	ivec2 grid = ivec2(floor(20.0 * in_uv));
	float checkers = float((grid.x + grid.y) % 2); 
	vec4 background = vec4(vec3(mix(0.3, 0.7, checkers)), 0.5f);
	
	out_color = mix(background, out_color, out_color.a);
	//out_color = vec4(out_color.a); 
}