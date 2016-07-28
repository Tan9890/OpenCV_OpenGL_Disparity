#version 400 core
// Input Color from vertex shader
in vec3 fragmentColor;

// Output fragment color
out vec3 color;


void main(){
  color = fragmentColor;
}