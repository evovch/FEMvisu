#version 400

layout(lines) in;
layout(line_strip, max_vertices=2) out;

in vec4 ex_Color[];
flat out vec4 geom_Color;

void main()
{
	for (int i=0; i<2; i++) {
		geom_Color = ex_Color[i];
		gl_Position = gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}