#version 400

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in vec4 vertex_Color[];
flat out vec3 geom_Color;

// For transform feedback
out vec3 additional_Position;
out vec3 additional_Color;

// Model view projection matrix - combination of model-to-camera, camera-to-clip
uniform mat4 MVP;

void main()
{
	for (int i=0; i<3; i++)
	{
			  geom_Color = vertex_Color[i].xyz;					gl_Position = MVP * gl_in[i].gl_Position;
		additional_Color = vertex_Color[i].xyz;		additional_Position = gl_Position.xyz / gl_Position.w;
		EmitVertex();
	}
	
	EndPrimitive();
}