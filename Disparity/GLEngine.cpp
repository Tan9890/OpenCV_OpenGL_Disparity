#include <iostream>
#include <fstream>
#include <math.h>
#include "GLEngine.h"
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"

using namespace std;

int GLEngine::InitializeGL(int width, int height, char* window_title)
{
	// Initialize the width and height for the OpenGL window
	win_width = width;
	win_height = height;

	// Error check for GLFW initialization
	if (glfwInit() == false)
	{
		cerr << "GLFW Initialization failed." << endl;
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4); // Enable Anti-Aliasing by 4

	// Set OpenGL version to 4.0
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	
	// Enable Core Profile for backwards compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create GLFW window
	window = glfwCreateWindow(win_width, win_height, window_title, NULL, NULL);

	//Window creation error check
	if (!window)
	{
		cerr << "Window creation failed." << endl;
		glfwTerminate();
		return -1;
	}

	// Set context for the current window, so that all operations are rendered in the specified window
	glfwMakeContextCurrent(window);
	
	// Required for core profile 
	glewExperimental = true;

	// GLEW initialization check
	if (glewInit() != GLEW_OK)
	{
		cerr << "GLEW Initialization failed" << endl;
		glfwTerminate();
		return -1;
	}

	// Allow keys and mouse input in the current window
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize mouse
	glfwPollEvents();
	glfwSetCursorPos(window, win_width / 2, win_height / 2);

	// Initialize Blue background for debugging purposes
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);

	// Render closest fragment
	glDepthFunc(GL_LESS);

	// Cull geometry that has its normal facing away from the camera i.e. angle between camera line of sight and face normal should be less tha 90 degrees
	glEnable(GL_CULL_FACE);
	return 0;
}


void GLEngine::GeneratePointCloud(cv::Mat DisparityMap)
{
	// Initialize Vertex Array Object
	glGenVertexArrays(1, &vaID);
	glBindVertexArray(vaID);

	// Serialized storage of 3D vertex coordinates in a vector obtained from the Disparity Map
	for (int i = 0; i < DisparityMap.rows; i++)
	{
		for (int j = 0; j < DisparityMap.cols; j++)
		{
			vertices.push_back((GLfloat)(DisparityMap.at<cv::Vec3f>(i, j)[0] * 0.25));
			vertices.push_back((GLfloat)(-DisparityMap.at<cv::Vec3f>(i, j)[1] * 0.25));
			vertices.push_back((GLfloat)(-DisparityMap.at<cv::Vec3f>(i, j)[2] * 0.25));
		}
	}

	// Initialize Vertex Buffer Object for the vertices
	glGenBuffers(1, &PCvertices);
	glBindBuffer(GL_ARRAY_BUFFER, PCvertices);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);	
}

// Method for loading PPM/PBM image as a texture
int GLEngine::loadPPMTex(const char * filepath)
{
	// Initialize file stream
	ifstream ifs;
	ifs.open(filepath, ios::binary);

	// Error check for missing/corrupt file stream
	if (ifs.fail())
	{
		cerr << "failed to open PPM file" << endl;
		return -1;
	}

	string header;
	ifs >> header;
	// Error check for header being a valid PPM/PBM file
	if (strcmp(header.c_str(), "P6") != 0)
	{
		cerr << "Can't read input file" << endl;
		return -1;
	}

	int w, h, b; // image width, height and bands/channels
	ifs >> w >> h >> b;

	// Ignore next line delimiter
	ifs.ignore(256, '\n');

	// Initialize vector for raw pixel data storage
	unsigned char px[3];
	vector<GLfloat> rawData;

	// Push pixel data serially in the vector; Data is represented as serial RGB values for every pixel
	for (int i = 0; i < w * h; i++)
	{
		ifs.read(reinterpret_cast<char*>(px), 3);
		rawData.push_back(px[0] / 255.0f);
		rawData.push_back(px[1] / 255.0f);
		rawData.push_back(px[2] / 255.0f);
	}
	ifs.close();

	// Create Vertex Buffer object for the pixel colors
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, rawData.size()*sizeof(GLfloat), &rawData[0], GL_STATIC_DRAW);

	return 0;
}

// Method for loading vertex and fragment shaders
GLuint GLEngine::LoadShaders(const char * vertexshader_path, const char * fragmentshader_path)
{
	// Unique IDs for vertex and fragment shader programs
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// load Vertex Shader file
	string VS_Code;
	ifstream VS_Stream(vertexshader_path, ios::in);
	if (VS_Stream.is_open()) {
		string Line = "";
		while (getline(VS_Stream, Line))
			VS_Code += "\n" + Line;
		VS_Stream.close();
	}
	else {
		cerr << "Unable to open Vertex Shader code file." << endl;
		return 0;
	}

	// Load Fragment Shader file
	string FS_Code;
	ifstream FS_Stream(fragmentshader_path, ios::in);
	if (FS_Stream.is_open()) {
		string Line = "";
		while (getline(FS_Stream, Line))
			FS_Code += "\n" + Line;
		FS_Stream.close();
	}
	else {
		cerr << "Unable to open Fragment Shader code file." << endl;
		return 0;
	}

	GLint compile_stat = GL_FALSE;
	int infoLogLength;

	// Compile Vertex Shader
	cout << "Compiling Vertex Shader..." << endl;
	char const * VSptr = VS_Code.c_str();
	glShaderSource(VertexShaderID, 1, &VSptr, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader Status
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &compile_stat);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 1) {
		std::vector<char> VS_compile_err_msg(infoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, infoLogLength, NULL, &VS_compile_err_msg[0]);
		cerr << &VS_compile_err_msg[0] << endl;
	}
	else
		cout << "Done." << endl;
	

	// Compile Fragment Shader
	cout << "Compiling Fragment Shader..." << endl;
	char const * FSptr = FS_Code.c_str();
	glShaderSource(FragmentShaderID, 1, &FSptr, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader Status
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &compile_stat);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 1) {
		std::vector<char> FS_compile_err_msg(infoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, infoLogLength, NULL, &FS_compile_err_msg[0]);
		cerr << &FS_compile_err_msg[0] << endl;
	}
	else
		cout << "Done." << endl;
	

	// Link Shaders to the program
	cout << "Linking Shaders..." << endl;
	GLuint Prog_ID = glCreateProgram();
	glAttachShader(Prog_ID, VertexShaderID);
	glAttachShader(Prog_ID, FragmentShaderID);
	glLinkProgram(Prog_ID);

	// Check the program
	glGetProgramiv(Prog_ID, GL_LINK_STATUS, &compile_stat);
	glGetProgramiv(Prog_ID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 1) {
		std::vector<char> proglink_err_msg(infoLogLength + 1);
		glGetProgramInfoLog(Prog_ID, infoLogLength, NULL, &proglink_err_msg[0]);
		cerr << proglink_err_msg[0] << endl;
	}
	else
		cout << "Done." << endl;

	// Free memory
	glDetachShader(Prog_ID, VertexShaderID);
	glDetachShader(Prog_ID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return Prog_ID;
}

glm::mat4 GLEngine::getViewMatrix()
{
	return view_matrix;
}

glm::mat4 GLEngine::getProjectionMatrix()
{
	return projection_matrix;
}

void GLEngine::Compute_Walk()
{
	// Initial call to glfwGetTime
	static double lastTime = glfwGetTime();

	// Compute delta time
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Reset mouse position for next frame
	glfwSetCursorPos(window,  win_width / 2, win_height / 2);

	// Calculate new mouse orientation
	x_angle += mousespeed * float(win_width / 2 - xpos);
	y_angle += mousespeed * float(win_height / 2 - ypos);

	// Spherical coordinates to Cartesian coordinates conversion to get direction
	glm::vec3 direction(cos(y_angle) * sin(x_angle), sin(y_angle), cos(y_angle) * cos(x_angle));

	// Right vector
	glm::vec3 right = glm::vec3(sin(x_angle - 3.14f / 2.0f),0,cos(x_angle - 3.14f / 2.0f)	);

	// Up vector
	glm::vec3 up = glm::cross(right, direction);

	// Movement using WASD keys
	// Move forward
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		cam_pos += direction * deltaTime * movespeed;
	}
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		cam_pos -= direction * deltaTime * movespeed;
	}
	// Strafe right
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		cam_pos += right * deltaTime * movespeed;
	}
	// Strafe left
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		cam_pos -= right * deltaTime * movespeed;
	}

	// Projection matrix : 45 degree Field of View, 4:3 aspect ratio, display range for near and far planes
	projection_matrix = glm::perspective(glm::radians(fov), 4.0f / 3.0f, 0.1f, 100.0f);

	// Camera matrix calibration for looking in a direction
	view_matrix = glm::lookAt(cam_pos, cam_pos + direction, up );

	// Set last time for the next frame
	lastTime = currentTime;
}

// Render method for displaying Point Cloud along with the left disparity texture
void GLEngine::Render(GLuint ShaderProgram, GLuint MatrixID)
{
	do {
		// Clear color buffer bit with bitwise or for creating the depth bit thus refreshing the entire screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use compiled shader program
		glUseProgram(ShaderProgram);

		// Take Walk around Inputs (keyboard and mouse) to compute the matrices
		Compute_Walk();

		// Generate Model View Projection (MVP) matrix
		glm::mat4 projection_matrix = getProjectionMatrix();
		glm::mat4 view_matrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = projection_matrix * view_matrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// GPU Attributes for interpreting vertex data
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, PCvertices);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// 0th layer with an increment of 3 for X,Y,Z coordinates

		// GPU Attributes for interpreting pixel color data
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// 1st layer with an increment of 3 for R,G,B pixel values

		glPointSize(4);														// Set point size to 4 for better visibility
		glDrawArrays(GL_POINTS, 0, vertices.size());						// Render vertices as points

		glDisableVertexAttribArray(0);										// Deallocate point cloud VBO
		glDisableVertexAttribArray(1);										// Deallocate pixel color VBO
		glfwSwapBuffers(window);											// Swap buffers for interchanging draw and render calls
		glfwPollEvents();													// Poll events of key presses
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
}