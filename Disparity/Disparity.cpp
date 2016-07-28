#include "Disparity.h"
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"

using namespace cv;
using namespace std;

// Method to calculate the left disparity
Mat Disparity::Calculate_Disparity(string left, string right)
{
	// read left end right stereo images in grayscale mode
	Mat lefteye = imread(left, IMREAD_GRAYSCALE);
	Mat righteye = imread(right, IMREAD_GRAYSCALE);

	// Initislize Disparity map with a 3 channel 32 bit floating point storage matrix
	cv::Mat imgDisparity32F = Mat(lefteye.rows, lefteye.cols, CV_32FC3);

	// Compute Disparity using Semi-Global Block Matching algorithm.

	// Heuristically chosen values for constructor:
	// minDisparity: -11
	// number of disparities = 16*7
	// block size = 3
	// P1 = 200
	// P2 = 2500
	// disp12MaxDiff = 40
	// prefilterCap = 63
	// uniqueness ratio = 0
	// speckle window size = 1
	// speckle range = 32
	// mode
	Ptr<StereoSGBM> sgbm = StereoSGBM::create(-11, 16 * 7, 3, 200, 2500, 40, 63, 0, 1, 32, StereoSGBM::MODE_SGBM);
	sgbm->compute(lefteye, righteye, imgDisparity32F);

	// Normalizing the disparity map and flattening it to a grayscale unsigned 8 bit matrix for display purposes
	double minv, maxv;
	minMaxLoc(imgDisparity32F, &minv, &maxv);
	Mat disparity8 = Mat(lefteye.rows, lefteye.cols, CV_8UC1);
	imgDisparity32F.convertTo(disparity8, CV_8UC1, 255 / (maxv - minv));
	imwrite(left + "disparity.png", disparity8);
	namedWindow("dsp", 0);
	imshow("dsp", disparity8);
	waitKey(10);

	// heuristically chosen Q matrix for 3d point cloud reconstruction
	Mat Q = (Mat_<double>(4, 4) <<	1.0, 0.0, 0.0, -160,			// 1	0	0		-cx
									0.0, 1.0, 0.0, -120,			// 0	1	0		-cy	
									0.0, 0.0, 0, 360.0,				// 0	0	0		f
									0.0, 0.0, 1.0 / 95, 0);			// 0	0	-1/Tx	(cx-cx')/Tx				

	// Reproject image to 3D Point Cloud
	cv::Mat XYZ(imgDisparity32F.size(), CV_32FC3);
	reprojectImageTo3D(imgDisparity32F, XYZ, Q, false, CV_32F);

	return XYZ;

}

