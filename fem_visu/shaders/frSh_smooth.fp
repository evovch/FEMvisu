#version 400

in vec4 geom_Color;
out vec4 out_Color;

void main()
{
	out_Color = geom_Color;
}