// dip_demos.cpp : Defines the entry point for the console application.
//

#include <iostream>

#include "utilities.h"
#include "Drawing.h"

#include "basicTrans.h"
#include "convolution.h"

// argc: arguments count,  3
// argv: arguments value,  dip_demos.exe d:\\Works\\cameraman.bmp  15

int main(int argc, char *argv[])
{
	std::cout<<"start..."<<std::endl;

	if(argc < 2)
	{
		std::cout<<"no args!"<<std::endl;
		return -1;
	}
	
	std::cout<<"arguments count:"<<argc<<std::endl;

	for(int arg_index=0; arg_index<argc; arg_index++)
		std::cout<<"argv["<<arg_index<<"]: "<<argv[arg_index]<<std::endl;


	std::string dir = argv[1];
	if (!dir.empty() && dir.back() != '\\')
		dir += '\\';
	// std::cout<<"argv[1]: "<<dir<<std::endl;

	std::string file_name = argv[2];
	// std::cout<<"argv[2]: "<<file_name<<std::endl;
	
	//第四次课-练习代码1：直方图均衡化 用于图像增强
	/*
	return cvrsg::kongzi::transform::HistEqual(dir + file_name);
	//*/
	
	//第四次课-练习代码2： 图像几何变换 - 仿射变换
	/*
	return cvrsg::kongzi::transform::WarpImageUinsgAffineExercise(
	  	dir + file_name, 
	  	atof(argv[3]), // angle_deg
	  	atof(argv[4]), // trans_x
	  	atof(argv[5]), // trans_y
	  	atof(argv[6]), // scale_x
	  	atof(argv[7])); // scale_y
	//*/

	//第四次课-练习代码3：计算仿射变换并进行几何变换
	/*
	return cvrsg::kongzi::transform::CalcTransfromAndWarpExercise(dir + file_name);
	//*/
	
	//第四次课-练习代码4： 图像平滑 - 2D卷积
	///*
	cvrsg::kongzi::conv::blurImageUsing2DConv(dir + file_name);
	//*/

	//第四次课-练习代码5： 图像平滑 - 1D卷积
	///*
	return cvrsg::kongzi::conv::blurImageUsing1DConv(dir + file_name);
	//*/

	return 0;
}
