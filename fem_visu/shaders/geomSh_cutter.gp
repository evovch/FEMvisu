#version 400

layout(triangles) in;
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

	vec3 B;	vec3 B_color;
	vec3 C;	vec3 C_color;
	
	double K;
	double T[3];
	double t;
	
	int numOfCrosses = 0;
	vec4 v_crossPosition[3];
	vec4 v_crossColors[3];
	
	bool visibleTriangle = true;

	if (dot(gl_in[0].gl_Position.xyz - A, n) > 0.0 ||
		dot(gl_in[1].gl_Position.xyz - A, n) > 0.0 ||
		dot(gl_in[2].gl_Position.xyz - A, n) > 0.0)
		visibleTriangle = false;

	if (visibleTriangle) {
	
		for (int i=0; i<3; i++) {
			      geom_Color = vertex_Color[i];				gl_Position = MVP *   gl_in[i].gl_Position;
			additional_Color = vertex_Color[i].xyz;			additional_Position = gl_in[i].gl_Position.xyz;
			EmitVertex();
		}
		EndPrimitive();
		
	} else {

		// Danger!!! Magic code. Don't even try to understand it. Even me, I don't really believe that it works.

		for (int i=0; i<3; i++) {
			B = gl_in[i].gl_Position.xyz;			B_color = vertex_Color[i].xyz;
			C = gl_in[(i+1)%3].gl_Position.xyz;		C_color = vertex_Color[(i+1)%3].xyz;
			K = dot(C-B, n);
			T[i] = dot(B-A, n);
			if (K != 0.0) {
				t = -T[i]/K;
				if (t >= 0.0 && t<= 1.0) {
					v_crossPosition[i] = vec4((C-B)*t+B, 1.0);
					v_crossColors[i] = vec4((C_color-B_color)*t+B_color, 1.0);
					numOfCrosses++;
				}
			}
		}
		
		if (numOfCrosses == 2) {
		
			int negativeI = -1;
			
			if (T[0] < 0.0 && T[1] > 0.0 && T[2] > 0.0) {
				negativeI = 0;
			} else if (T[0] > 0.0 && T[1] < 0.0 && T[2] > 0.0) {
				negativeI = 1;
			} else if (T[0] > 0.0 && T[1] > 0.0 && T[2] < 0.0) {
				negativeI = 2;
			}
			if (negativeI > -1) { // The plane leaves a triangle piece visible
				      geom_Color = vertex_Color[negativeI];					gl_Position = MVP *   gl_in[negativeI].gl_Position;
				additional_Color = vertex_Color[negativeI].xyz;				additional_Position = gl_in[negativeI].gl_Position.xyz;
				EmitVertex();
				
				      geom_Color = v_crossColors[(negativeI+2)%3];			gl_Position = MVP *   v_crossPosition[(negativeI+2)%3];
				additional_Color = v_crossColors[(negativeI+2)%3].xyz;		additional_Position = v_crossPosition[(negativeI+2)%3].xyz;
				EmitVertex();
				
				      geom_Color = v_crossColors[negativeI];				gl_Position = MVP *   v_crossPosition[negativeI];
				additional_Color = v_crossColors[negativeI].xyz;			additional_Position = v_crossPosition[negativeI].xyz;
				EmitVertex();
				
				EndPrimitive();
			} else {

				int positiveI = -1;
				
				if (T[0] > 0.0 && T[1] < 0.0 && T[2] < 0.0) {
					positiveI = 0;
				} else if (T[0] < 0.0 && T[1] > 0.0 && T[2] < 0.0) {
					positiveI = 1;
				} else if (T[0] < 0.0 && T[1] < 0.0 && T[2] > 0.0) {
					positiveI = 2;
				}
				if (positiveI > -1) { // The plane leaves a quad piece visible
					      geom_Color = vertex_Color[(positiveI+1)%3];			gl_Position = MVP *   gl_in[(positiveI+1)%3].gl_Position;
					additional_Color = vertex_Color[(positiveI+1)%3].xyz;		additional_Position = gl_in[(positiveI+1)%3].gl_Position.xyz;
					EmitVertex();
					
					      geom_Color = vertex_Color[(positiveI+2)%3];			gl_Position = MVP *   gl_in[(positiveI+2)%3].gl_Position;
					additional_Color = vertex_Color[(positiveI+2)%3].xyz;		additional_Position = gl_in[(positiveI+2)%3].gl_Position.xyz;
					EmitVertex();
					
					      geom_Color = v_crossColors[positiveI];				gl_Position = MVP *   v_crossPosition[positiveI];
					additional_Color = v_crossColors[positiveI].xyz;			additional_Position = v_crossPosition[positiveI].xyz;
					EmitVertex();
					
					      geom_Color = v_crossColors[(positiveI+2)%3];			gl_Position = MVP *	  v_crossPosition[(positiveI+2)%3];
					additional_Color = v_crossColors[(positiveI+2)%3].xyz;		additional_Position = v_crossPosition[(positiveI+2)%3].xyz;
					EmitVertex();
					
					EndPrimitive();
				}
				
			}
		
		}

	}

}