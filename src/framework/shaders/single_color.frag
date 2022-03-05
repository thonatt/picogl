#version 460

layout(location = 0) out vec4 color;

uniform vec4 uniform_color;

void main()
{
	color = uniform_color;
}