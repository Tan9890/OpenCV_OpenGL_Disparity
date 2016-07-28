#pragma once
#ifndef DISPARITY_H
#define DISPARITY_H

#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"

using namespace std;


class Disparity
{
public:
	static cv::Mat Calculate_Disparity(string left, string right);
};

#endif //DISPARITY_H

