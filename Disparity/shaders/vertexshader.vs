#version 400 core

// Interpret specified variables in the buffer levels.
layout(location = 0) in vec3 vertexPosition_MS;
layout(location = 1) in vec3 vertexColor;

// Output color for each fragment taken from the vertexcolor
out vec3 fragmentColor;

// MVP matrix for camera view based position caclulation
uniform mat4 MVP;

void main(){

	// Output position 
	gl_Position =  MVP * vec4(vertexPosition_MS,1);
	
	// output fragment color from the RGB values in vertex color
	fragmentColor = vertexColor;
}