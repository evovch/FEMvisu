#version 400

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices=4) out;

in vec4 vertex_Color[];
flat out vec4 geom_Color;

// For transform feedback
out vec3 additional_Position;
out vec3 additional_Color;

// Model view projection matrix - combination of model-to-camera, camera-to-clip
uniform mat4 MVP;

uniform vec3 sectionOrigin;
uniform vec3 sectionNorm;

void main()
{
	vec3 A = sectionOrigin;
	vec3 n = sectionNorm;

	vec3 B; vec3 B_color;
	vec3 C; vec3 C_color;

	double K;
	double T;
	double t;

	int numOfCrosses = 0;
	vec4 v_crossPosition[6];
	vec4 v_crossColors[6];

	int ii[12];
	ii[0] = 0;	ii[1] = 1;	ii[2] = 0;	ii[3] = 2;
	ii[4] = 0;	ii[5] = 3;	ii[6] = 1;	ii[7] = 2;
	ii[8] = 1;	ii[9] = 3;	ii[10] = 2;	ii[11] = 3;

	for (int i=0; i<6; i++) {
		B = gl_in[ii[i*2+0]].gl_Position.xyz; B_color = vertex_Color[ii[i*2+0]].xyz;
		C = gl_in[ii[i*2+1]].gl_Position.xyz; C_color = vertex_Color[ii[i*2+1]].xyz;
		K = dot(C-B, n);
		T = dot(B-A, n);
		if (K != 0.0) {
			t = -T/K;
			if (t >= 0.0 && t<= 1.0) {
				v_crossPosition[numOfCrosses] = vec4((C-B)*t+B, 1.0);
				v_crossColors[numOfCrosses] = vec4((C_color-B_color)*t + B_color, 1.0);
				numOfCrosses++;
			}
		}
		//TODO test
		// This case is when line BC is on the section plane
		// Beware of exact comparison of two 'double's
		/*
		if (K == 0.0 && T == 0.0) {
			v_crossPosition[numOfCrosses] = vec4(B, 1.0);
			numOfCrosses++;
			v_crossPosition[numOfCrosses] = vec4(C, 1.0);
			numOfCrosses++;
		}
		*/
	}

	vec4 v_color = vec4(0.9, 0.2, 0.3, 1.0); 

	if (numOfCrosses == 3)
	{
		      geom_Color = v_crossColors[0];		gl_Position = MVP *   v_crossPosition[0];
		additional_Color = v_crossColors[0].xyz;	additional_Position = v_crossPosition[0].xyz;
		EmitVertex();
		
		      geom_Color = v_crossColors[1];		gl_Position = MVP *   v_crossPosition[1];
		additional_Color = v_crossColors[1].xyz;	additional_Position = v_crossPosition[1].xyz;
		EmitVertex();
		
		      geom_Color = v_crossColors[2];		gl_Position = MVP *   v_crossPosition[2];
		additional_Color = v_crossColors[2].xyz;	additional_Position = v_crossPosition[2].xyz;
		EmitVertex();
		
		EndPrimitive();	
	}
	else if (numOfCrosses == 4)
	{
		      geom_Color = v_crossColors[0];		gl_Position = MVP *   v_crossPosition[0];
		additional_Color = v_crossColors[0].xyz;	additional_Position = v_crossPosition[0].xyz;
		EmitVertex();
		
		      geom_Color = v_crossColors[1];		gl_Position = MVP *   v_crossPosition[1];
		additional_Color = v_crossColors[1].xyz;	additional_Position = v_crossPosition[1].xyz;
		EmitVertex();
		
		      geom_Color = v_crossColors[2];		gl_Position = MVP *   v_crossPosition[2];
		additional_Color = v_crossColors[2].xyz;	additional_Position = v_crossPosition[2].xyz;
		EmitVertex();
		
		      geom_Color = v_crossColors[3];		gl_Position = MVP *   v_crossPosition[3];
		additional_Color = v_crossColors[3].xyz;	additional_Position = v_crossPosition[3].xyz;
		EmitVertex();
		
		EndPrimitive();
	}

}
