#version 460

layout(location = 0) out vec4 color;

in VertexData {
	vec3 position, normal, color;
	vec2 uv;
} frag_in;

// https://www.shadertoy.com/view/3tVGRz
float checkers(vec2 p)
{
	return float((int(floor(p.x))^int(floor(p.y)))&1);
}

void main()
{
	vec2 uv = frag_in.position.xz;
	
	float x = checkers(uv);
	
	vec2 d_measure = fwidth(uv);
	
	vec2 fuv = fract(uv);
	vec2 b_measure = 0.5 - abs(fuv - 0.5);

	vec2 aa_uv =  1.0 - b_measure / (d_measure * (1.0 + 16.0 * d_measure));
	float antialising = clamp(max(aa_uv.x, aa_uv.y), 0.0, 1.0);
	
	color = vec4(vec3(mix(x, 0.5, antialising)), 0.5);
}