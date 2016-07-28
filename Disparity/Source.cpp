#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"
#include "Disparity.h"
#include "GLEngine.h"


using namespace std;

// The process starts with calculating the Disparity Matrix from the stereo images using the SGBM algorithm, followed by its 3D point cloud reconstruction.
// The 3D Point Cloud is passed to OpenGL as individual vertices and is displayed with points.
// Since there is a 1 to 1 correspondence between the point cloud vertices and the pixel colors in the image, i.e. there are m * n 3D points for an m * n
// image there is no need for implementing a UV based texture mapping; instead each RGB value is passed as a separate buffer for each XYZ co-ordinate.
// The advantage is minification and magnification have no effect on the vertex color, since each point has one solid color grabbed from the image 
// at all times.

void main() {

	GLEngine engine;
	engine.InitializeGL(1024, 768, "Disparity");
	// Cones Images
	//cv::Mat DisparityMap = Disparity::Calculate_Disparity("./Images/im2_q.ppm", "./Images/im6_q.ppm");
	//engine.loadPPMTex("./Images/im2_q.ppm");
	
	// Own captured images
	//cv::Mat DisparityMap = Disparity::Calculate_Disparity("./Images/l.pbm", "./Images/r.pbm"); 
	//engine.loadPPMTex("./Images/l.pbm");

	// Stereo sample image from the internet
	cv::Mat DisparityMap = Disparity::Calculate_Disparity("./Images/tigl.pbm", "./Images/tigr.pbm");
	engine.loadPPMTex("./Images/tigl.pbm");

	GLuint ShaderProgram = engine.LoadShaders("./shaders/vertexshader.vs", "./shaders/fragmentshader.fs");
	GLuint MatrixID = glGetUniformLocation(ShaderProgram, "MVP");
	engine.GeneratePointCloud(DisparityMap);
	engine.Render(ShaderProgram, MatrixID);
}