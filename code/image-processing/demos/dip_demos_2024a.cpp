// dip_demos.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <vector>

// #include "opencv.hpp"
#include "utilities.h"
// #include "convolution2024A.h"

// argc: arguments count,  3
// argv: arguments value,  dip_demos.exe d:\\Works\\cameraman.bmp  15

int main(int argc, char *argv[])
{
	std::cout << "Start digital image processing..." << std::endl;
	std::cout << std::endl;
	
	if (argc < 3)
	{
		std::cout << "Error: No args, Please input directory and filename!" << std::endl;
		return -1;
	}

	std::cout << "arguments count:" << argc << std::endl;

	for (int arg_index = 0; arg_index < 5; arg_index++)
		std::cout << "argv[" << arg_index << "]: " << argv[arg_index] << std::endl;

	std::cout<< std::endl;

	/* float array32f[] = {0, 1, 2, 3, 4, 5, 6, 7};
	// cv::Mat temp_mat = cv::Mat(1, 8, CV_32FC1, array32f);
	// std::cout << temp_mat << std::endl;*/ 

	std::string dir = argv[1];
	std::string image_name = argv[2];
	std::string image_path = dir + image_name;

	std::string data_name = argv[3];
	std::string data_path = dir + data_name;

	std::string task = cvrsg::common::utilities::toLower( argv[4]);
	// if( task == "points")
	// 	return DIPAutumn2024::geoTransViaPoints(image_path, data_path);
	// if( task == "matrix")
	// 	return DIPAutumn2024::geoTransViaAffineMatrix(image_path, data_path); 
	// if( task == "params")
	// 	return DIPAutumn2024::geoTransViaAngleScaleTrans(image_path, data_path); 

	// std::string task = cvrsg::common::utilities::toLower( argv[4]);
	// if(task == "conv2d")
	// 	conv2024A::conv2d(image_path, data_path);
	// if(task == "conv2d_separated")
	// 	conv2024A::conv2dSeparated_onclass(image_path, data_path);
	// if(task == "median")
	// 	conv2024A::medianFilter_onclass(image_path);

	return 0;
}
