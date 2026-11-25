// dip_demos.cpp : Defines the entry point for the console application.
//

#include <iostream>

#include "utilities.h"
#include "Drawing.h"

#include "edge.h"
#include "basicTrans.h"
#include "convolution.h"
#include "corners.h"

// argc: arguments count,  3
// argv: arguments value,  dip_demos.exe d:\\Works\\cameraman.bmp  15

int main(int argc, char *argv[])
{
	std::cout<<"start..."<<std::endl;

	// 至少要有两个参数：图像所在目录，以及图像文件名。
	// 例如：dip_demos.exe data\ cameraman.bmp
	if(argc < 3)
	{
		std::cout<<"用法：dip_demos.exe 图像目录 图像文件名"<<std::endl;
		std::cout<<"这一次演示调用 Canny 边缘检测，两个阈值是 60 和 100。"<<std::endl;
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
	
	// 练习1：直方图均衡化，用来增强灰度对比度。
	// 先统计原图每个灰度级出现的次数，再求累积直方图，最后按累积比例把灰度重新映射到 0～255。
	// 结果是暗部变亮、亮部拉开，整幅图的灰度分布更接近均匀。只需要图像路径一个参数。
	/*
	return cvrsg::kongzi::transform::HistEqual(dir + file_name);
	//*/

	// 练习2：按给定参数构造仿射变换，并做几何变换。
	// 命令行还要再跟五个数：旋转角（单位是度）、水平平移、竖直平移、水平缩放、竖直缩放。
	// 例如：dip_demos.exe data\ cameraman.bmp 30 20 10 1.2 0.8
	// 函数用这些量填入 2×3 仿射矩阵，再调用 warpAffine 得到变换后的图像。
	/*
	return cvrsg::kongzi::transform::WarpImageUinsgAffineExercise(
	  	dir + file_name,
	  	atof(argv[3]), // 旋转角，单位是度
	  	atof(argv[4]), // 水平平移，单位是像素
	  	atof(argv[5]), // 竖直平移，单位是像素
	  	atof(argv[6]), // 水平方向缩放倍数
	  	atof(argv[7])); // 竖直方向缩放倍数
	//*/

	// 练习3：用三对同名点估计仿射变换，再对图像做几何变换。
	// 函数内部固定了原图上三个点和目标位置上三个点，先用 getAffineTransform 求出 2×3 矩阵，
	// 再求逆矩阵并用逆向映射把原图像素填到新画布上。只需要图像路径一个参数。
	/*
	return cvrsg::kongzi::transform::CalcTransfromAndWarpExercise(dir + file_name);
	//*/

	// 练习4：用二维卷积做图像平滑。
	// 先用一维权重 [1, 4, 6, 4, 1] 外积得到 5×5 卷积核，再把核系数之和归一成 1，
	// 然后对整幅图做二维卷积。邻域加权平均会减弱噪声，图像变模糊。只需要图像路径一个参数。
	/*
	cvrsg::kongzi::conv::blurImageUsing2DConv(dir + file_name);
	//*/

	// 练习5：用可分离的一维卷积做同样的平滑。
	// 同一个权重 [1, 4, 6, 4, 1] 先沿行卷积，再把核转置后沿列卷积，结果应与练习4 的二维卷积接近，
	// 但运算量更小。控制台会打印耗时，可与练习4 对照。只需要图像路径一个参数。
	/*
	return cvrsg::kongzi::conv::blurImageUsing1DConv(dir + file_name);
	//*/

	// 本次演示：Canny 边缘检测。
	// 第一个参数是待检测的灰度图像路径。
	// 后两个数是梯度幅度上的两个阈值，单位与索贝尔（Sobel）梯度幅度一致。
	// 函数内部会先做高斯平滑，再计算梯度，然后做非极大值抑制。
	// 幅度不低于高阈值的像素保留为强边缘；介于低阈值和高阈值之间的像素先记为弱边缘，
	// 只有与强边缘相连时才保留。这里写成 60 和 100，函数里若高阈值小于低阈值会自动对调。
	// 运行后依次弹出原图、梯度幅度、梯度方向、非极大值抑制结果和最终边缘，按任意键进入下一幅。
	///*
	return cvrsg::kongzi::edge::canny(dir + file_name, 60, 100);
	//*/


	// return cvrsg::kongzi::edge::lesson::canny_2023spring(dir + file_name, 60, 100);

	// return cvrsg::kongzi::corners::harrisCorner(dir + file_name);
	// return cvrsg::kongzi::corners::multiScaleCorner(dir + file_name);
	// return cvrsg::kongzi::corners::multiScaleKeyPoints(dir + file_name);

	// compute direction using HOG====
	//  cv::Mat image = cv::imread(dir + file_name, cv::IMREAD_GRAYSCALE);
	//  std::vector<cv::Rect> ROI_list;
	//  ROI_list.push_back(cv::Rect(155, 162, 32, 32));
	//  ROI_list.push_back(cv::Rect(103, 19, 32, 32));
	//  ROI_list.push_back(cv::Rect(76, 162, 32, 32));
	//  ROI_list.push_back(cv::Rect(25, 100, 32, 32));
	//  ROI_list.push_back(cv::Rect(35, 68, 32, 32));
	//  ROI_list.push_back(cv::Rect(65, 50, 32, 32));

	// for (int i = 0; i < ROI_list.size(); i++)
	// {
	// 	cvrsg::kongzi::corners::findDirectionsUsingHOG(image, ROI_list[i]);
	// }
	// return 0;
	//============HOG===================

	// return cvrsg::kongzi::corners::siftCorners(dir + argv[2]);
	//return cvrsg::kongzi::corners::extractSIFTAndMatch(dir + argv[2], dir + argv[3]);

	// return cvrsg::kongzi::corners::siftMatch(dir + argv[2], dir + argv[3]);

	return 0;
}
