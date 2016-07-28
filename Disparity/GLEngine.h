#pragma once
#ifndef GLENGINE_H
#define GLENGINE_H

#include <iostream>
#include <fstream>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"

using namespace std;

class GLEngine
{
public:
	int InitializeGL(int width, int height, char* window_title);							// Initialize the GL display

	void GeneratePointCloud(cv::Mat DisparityMap);											// Generation of Point Cloud from reprojection matrix from OpenCV
	int loadPPMTex(const char* filepath);													// Use PPM/PBM images as textures

	GLuint LoadShaders(const char * vertexshader_path, const char * fragmentshader_path);	// Load Shaders from the specified filepath
	void GLEngine::Render(GLuint ShaderProgram, GLuint MatrixID);							// Render method for Point Cloud

	glm::mat4 GLEngine::getViewMatrix();													// Method to return the private View Matrix
	glm::mat4 GLEngine::getProjectionMatrix();												// Method to return the private Projection Matrix
	void Compute_Walk();																	// Allow walk around functionality using WASD keys and Mouse

private:
	int win_width, win_height;					// Window width and height
	GLFWwindow* window;							// Main Window for OpenGL

	GLuint vaID;						// VAO for verices and texture pixel data
	GLuint PCvertices;							// VBO for vertices
	GLuint colorbuffer;							// VBO for pixel color data

	vector<GLfloat> vertices;					// vertex data

	glm::mat4 view_matrix;
	glm::mat4 projection_matrix;
	glm::vec3 cam_pos = glm::vec3(0, 0, 5);		// Starting camera position on +Z direction
	float x_angle = 3.14f;						// Starting horizontal angle pointing in -Z direction
	float y_angle = 0.0f;						// Starting vertical angle
	float fov = 45.0f;							// Starting Field of View of camera 
	float movespeed = 15.0f;						// WASD key movement speed
	float mousespeed = 0.005f;					// Mouse Look speed

};
#endif //GLENGINE_H

