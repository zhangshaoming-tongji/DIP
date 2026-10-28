#ifndef _CVRSG_KONGZI_CONV_H
#define _CVRSG_KONGZI_CONV_H


#include <string>
#include "opencv.hpp"

namespace cvrsg
{
	namespace kongzi
	{

		namespace conv
		{
			
			//课堂代码：图像平滑 - 2D卷积
			int blurImageUsing2DConv(std::string path);
			
			//课堂代码：图像平滑 - 1D卷积
			int blurImageUsing1DConv(std::string path);

			//其他参考代码============================================
			void calcConv2D(const cv::Mat& src, cv::Mat& dst, const cv::Mat& kernel);
			void calcConv1D(const cv::Mat& src, cv::Mat& dst, const cv::Mat& kernel);
			
			int sharpeningImage1(std::string path);
			int sharpeningImage(std::string path);

			void rankFilter1(const cv::Mat& src, cv::Mat& dst, int ksize, int rank);
			int medianBlur2023Oct1(std::string path);

			void rankFilter(const cv::Mat& src, cv::Mat& dst, int ksize, int rank);		
			int medianBlur2023Oct(std::string path); 



			
			float getDefaultSigma(int ksize);


			// type of kernel is CV_32FC1
			// type of dst_image is CV_32FC1 too
			void conv2D(const cv::Mat &image, cv::Mat &dst_image, const cv::Mat &kernel);

			int blurUsingConv2D(std::string image_path);

			void conv1D(const cv::Mat &image, cv::Mat &dst_image, float *kernel, int ksize, bool conv_in_horizontal = true);

			int blurUsingConv1D(std::string image_path);

			void rankOrderFilter2D_speedUp(const cv::Mat& input, cv::Mat& output, int ksize, int order);
			void rankOrderFilter2D_dummy(const cv::Mat& input, cv::Mat& output, int ksize, int order);
			
			//20230331
			void rankOrderFilter2D_dummy1(const cv::Mat& input, cv::Mat& output, int ksize, int th);
			void rankOrderFilter2D_speedUp1(const cv::Mat& input, cv::Mat& output, int ksize, int th);
			
			int medianBlurEx(std::string path);
			int medianBlurDummy(std::string path);


			int medianBlur(std::string path);
			int medianBlur3Ways(std::string path);

			// void conv2D(const cv::Mat &image, cv::Mat &dst_image, const cv::Mat &kernel, int ker_size);

			// void conv1D(const cv::Mat &image, cv::Mat &dst_image, float *kernel, int ker_size, bool row_vector_flag = true);

			int computeGradient(std::string path);

			cv::Mat getGaussianKernel2D(int size = 3, double sigma = 1.0);

			inline int getKsizeFromSigma(float sigma);
		}
	}
}

#endif //_CVRSG_KONGZI_CONV_H