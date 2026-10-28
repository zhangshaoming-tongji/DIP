#include "convolution.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "utilities.h"

namespace cvrsg
{
	namespace kongzi
	{

		namespace conv
		{

			void calcConv2D(const cv::Mat &src, cv::Mat &dst, const cv::Mat &kernel)
			{
				//如果输出图像为空，则创建一个与输入图像大小相同的输出图像
				if (dst.empty())
					dst = cv::Mat::zeros(src.size(), CV_32FC1);

				//获取卷积核的尺寸	ksize = kernel.cols / 2
				int ksize = kernel.cols / 2;

				for (int row = 0; row < src.rows; row++)//遍历输入图像
				{
					for (int col = 0; col < src.cols; col++)//遍历输入图像
					{
						float conv_result = 0;//卷积结果
						for (int i = -ksize; i <= ksize; i++) // row of kernel 行卷积
						{
							for (int j = -ksize; j <= ksize; j++) // col of kernel 列卷积
							{
								int pixel_row = row + i;//当前像素的行索引
								int pixel_col = col + j;//当前像素的列索引

								//如果当前像素越界，则跳过
								if (pixel_col < 0 || pixel_col > src.cols - 1 ||
									pixel_row < 0 || pixel_row > src.rows - 1)
									continue;//如果当前像素越界，则跳过

								float pixel_value = src.at<uchar>(pixel_row, pixel_col);//获取当前像素的值
								float kernel_value = kernel.at<float>(i + ksize, j + ksize);//获取当前卷积核的值

								conv_result += pixel_value * kernel_value;//计算卷积结果
							}
						}
						dst.at<float>(row, col) = conv_result;//保存卷积结果
					}
				}
			}	

			int blurImageUsing2DConv(std::string path)
			{
				cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);//读取输入图像
				if (image.empty())
				{
					std::cout << "Reading file error in blur!" << std::endl;//读取输入图像失败
					return -1;
				}

				cv::Mat dst = cv::Mat(image.rows, image.cols, CV_32FC1);//创建输出图像

				// cv::Mat kernel = cv::Mat::ones(5, 5, CV_32FC1);

				cv::Mat kernel_column = (cv::Mat_<float>(5, 1)<< 1,4,6,4,1);//创建卷积核

				cv::Mat kernel  = kernel_column * kernel_column.t();//计算卷积核		

				float sum = cv::sum(kernel)[0];//计算卷积核的和

				kernel /= sum;//归一化卷积核

				// std::cout << "kernel:" << std::endl;
				// std::cout << kernel << std::endl;

				double start = static_cast<double>(cv::getTickCount()); //开始时间
				calcConv2D(image, dst, kernel);
				double end = static_cast<double>(cv::getTickCount()); //结束时间
				double duration = ((double)(end - start) / cv::getTickFrequency());

				std::cout << "Duration of 2d conv: " << duration << std::endl; //输出卷积时间

				cv::normalize(dst, dst, 0, 255, cv::NORM_MINMAX); //归一化输出图像
				dst.convertTo(dst, CV_8UC1); //转换为8位无符号整型图像	

				cv::imshow("image", image);
				cv::imshow("conv2d", dst);
				cv::waitKey(0);

				return 0;
			}

			void calcConv1D(const cv::Mat &src, cv::Mat &dst, const cv::Mat &kernel)
			{
				//如果输出图像为空，则创建一个与输入图像大小相同的输出图像
				if (dst.empty())
					dst = cv::Mat::zeros(src.size(), CV_32FC1);

				//获取卷积核的尺寸
				int ksize_row = kernel.rows;
				int ksize_col = kernel.cols;

				for (int row = 0; row < src.rows; row++)
				{
					for (int col = 0; col < src.cols; col++)
					{
						float conv_result = 0;

						if (ksize_row > 1)//conv in row 行卷积
						{
							for (int offset = -ksize_row / 2; offset <= ksize_row / 2; offset++)
							{//行卷积

								//获取当前像素的值
								float pixel_value = src.at<float>(row + offset, col + 0);
								//如果当前像素越界，则设置为0
								if (row + offset < 0)
									pixel_value = 0;
								if (row + offset > src.rows - 1)
									pixel_value = 0;//如果当前像素越界，则设置为0

								//获取当前卷积核的值
								float kernel_value = kernel.at<float>(offset + ksize_row / 2, 0);
								//计算卷积结果
								conv_result += pixel_value * kernel_value;
							}
						}
						else//conv in column 列卷积
						{
							//列卷积
							for (int offset = -ksize_col / 2; offset <= ksize_col / 2; offset++)
							{//列卷积
								//获取当前像素的值
								float pixel_value = src.at<float>(row + 0, col + offset);
								//如果当前像素越界，则设置为0
								if (col + offset < 0)
									pixel_value = 0;//如果当前像素越界，则设置为0
								if (col + offset > src.cols - 1)
									pixel_value = 0;//如果当前像素越界，则设置为0
								//获取当前卷积核的值
								float kernel_value = kernel.at<float>(0, offset + ksize_col / 2);
								//计算卷积结果
								conv_result += pixel_value * kernel_value;
							}
						}
						//保存卷积结果
						dst.at<float>(row, col) = conv_result;
					}
				}
			}

			int blurImageUsing1DConv(std::string path)
			{
				cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);//读取输入图像
				if (image.empty())
				{
					std::cout << "Reading file error in blur!" << std::endl;//读取输入图像失败
					return -1;
				}

				cv::Mat image_32f;//创建32位浮点型图像

				image.convertTo(image_32f, CV_32FC1);//转换为32位浮点型图像

				cv::Mat kernel = cv::Mat(5, 1, CV_32FC1);//创建卷积核
				kernel.at<float>(0, 0) = 1;//设置卷积核的值
				kernel.at<float>(1, 0) = 4;//设置卷积核的值
				kernel.at<float>(2, 0) = 6;//设置卷积核的值
				kernel.at<float>(3, 0) = 4;//设置卷积核的值
				kernel.at<float>(4, 0) = 1;//设置卷积核的值

				float sum = cv::sum(kernel)[0];//计算卷积核的和
				kernel /= sum;//归一化卷积核		

				// std::cout << kernel << std::endl;

				cv::Mat kernel_trans = kernel.t();//转置卷积核
				// std::cout << kernel_trans << std::endl;

				cv::Mat dst_row_32f = cv::Mat(image.rows, image.cols, CV_32FC1);//创建32位浮点型图像
				cv::Mat dst_32f = cv::Mat(image.rows, image.cols, CV_32FC1);//创建32位浮点型图像

				double start = static_cast<double>(cv::getTickCount()); //开始时间
				// conv in row
				calcConv1D(image_32f, dst_row_32f, kernel); //行卷积	
				// conv in column
				calcConv1D(dst_row_32f, dst_32f, kernel_trans); //列卷积
				double end = static_cast<double>(cv::getTickCount()); //结束时间
				double duration = ((double)(end - start) / cv::getTickFrequency());

				std::cout<< "Duration of conv 1d: "<< duration << std::endl; //输出卷积时间

				// std::cout << dst_32f << std::endl;

				cv::Mat dst;
				cv::normalize(dst_32f, dst, 0, 255, cv::NORM_MINMAX); //归一化输出图像
				dst.convertTo(dst, CV_8UC1); //转换为8位无符号整型图像	
				// std::cout << dst << std::endl;

				cv::imshow("image", image);
				cv::imshow("conv1d", dst);
				cv::waitKey(0);

				return 0;
			}

			//=======================================================================================


			void calcHist(const cv::Mat& src, std::vector<int>& hist)
			{
				hist = std::vector<int>(256,0); // int

				for(int row=0; row < src.rows; row++)
				{
					for( int col=0; col < src.cols; col++)
					{
						int gray = src.at<uchar>(row, col);
						hist[gray] ++;
					}
				}

			}

			int medianBlur2023Oct1(std::string path)
			{
				cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
				if (image.empty())
				{
					std::cout << "Reading file error in median blur!" << std::endl;
					return -1;
				}

				cv::Mat dst;
				int ksize = 3;
				int rank = ksize * ksize / 2 + 1;
				rankFilter1(image, dst, ksize, rank);

				cv::imshow("image", image);
				cv::imshow("dist", dst);
				
				cv::waitKey(0);

				return 0;

			}


			void rankFilter1(const cv::Mat& src, cv::Mat& dst, int ksize, int rank)
			{
				dst = cv::Mat(src.size(), CV_8UC1);

				int border = ksize / 2;
				for (int row = border; row < src.rows - border; row++)
				{
					std::vector<int> hist_patch; // ksize x ksize 大小的窗口的直方图
					uchar target_value;
					int num_lower; // 当前patch中 ≤ target_value的像素个数

					for(int col = border; col < src.cols -border; col++)
					{
						if(col == border)//initialization
						{
							int x = col - ksize/2;
							int y = row - ksize/2;
							cv::Mat patch = src(cv::Rect(x,y,ksize, ksize)).clone();

							std::vector<uchar> pixels = patch.reshape(1,1);
							std::sort(pixels.begin(), pixels.end()); //默认升序排序

							target_value = pixels[rank - 1];

							calcHist(patch, hist_patch);

							num_lower = 0;
							for (int gray_id = 0; gray_id <= target_value; gray_id++)
								num_lower += hist_patch[gray_id];
							
						}
						else //update
						{
							//update histogram of this patch
							for (int i = -ksize / 2; i <= ksize / 2; i++)
							{
								int gray = src.at<uchar>(row + i, col - ksize / 2 - 1);
								hist_patch[gray] -= 1;
								if (gray <= target_value)
									num_lower--;

								gray = src.at<uchar>(row + i, col + 1);
								hist_patch[gray] += 1;
								if (gray <= target_value)
									num_lower++;
							}

							if (num_lower < rank)
							{
								while (num_lower < rank)
								{
									target_value++;
									num_lower += hist_patch[target_value];
									// num_lower changed
								}
							}
							else // num_lower > rank
							{
								while (num_lower > rank)
								{
									num_lower -= hist_patch[target_value];
									target_value--;
									
								}
								

							}
						}

						dst.at<uchar>(row, col) = target_value;
					}
				}
			}

			void calcHist(const cv::Mat &src, cv::Mat &hist, int gray_levels)
			{
				hist = cv::Mat::zeros(gray_levels, 1, CV_32SC1); // int
		
				for (int row = 0; row < src.rows; row++)
				{
					for (int col = 0; col < src.cols; col++)
					{
						int gray = src.at<uchar>(row, col);
						hist.at<int>(gray, 0) += 1;
					}
				}
			}

			void rankFilter(const cv::Mat &src, cv::Mat &dst, int ksize, int rank)
			{
				if (dst.empty())
					dst = cv::Mat(src.size(), CV_8UC1);

				int border = ksize / 2;
				for (int row = border; row < src.rows - border; row++)
				{
					// 创建当前窗口的排序结果值
					uchar target_value;

					// 当前patch中灰度小于等于target_value的像素个数
					int num_lower;

					// 创建局部窗口直方图，供本行使用
					cv::Mat patch_hist;
					
					for (int col = border; col < src.cols - border; col++)
					{
						
						// 如果是第一列，则初始化局部窗口直方图
						if (col == border)
						{
							// 获取当前窗口的图像，大小为ksize x ksize
							int x = col - border;
							int y = row - border;
							cv::Rect roi(x, y, ksize, ksize);
							cv::Mat patch = src(roi).clone();

							int gray_levels = 256;
							calcHist(patch, patch_hist, gray_levels);

							// std::cout << patch << std::endl;
							// std::cout << patch_hist << std::endl;

							// 初始化sort_value和num_lower(patch中小于等于目标值的像素个数)
							std::vector<uchar> pixels = patch.reshape(0, 1);
							std::sort(pixels.begin(), pixels.end()); // 默认升序排列
							target_value = pixels.at(rank - 1);

							num_lower = 0;
							for (int i = 0; i < gray_levels; i++)
							{
								if (i <= target_value)
									num_lower += patch_hist.at<int>(i,0);
							}

							// end of init patch
						}
						else // 如果不是第一列，则更新局部窗口直方图patch_hist
						{
							// 更新patch的统计信息
							for (int i = -ksize / 2; i <= ksize / 2; i++)
							{
								// 将原patch中最左侧的列从patch统计信息中移除
								int gray = src.at<uchar>(row + i, col - ksize / 2 - 1);
								patch_hist.at<int>(gray, 0) -= 1;
								if (gray <= target_value)
									num_lower--;

								// 将原patch右侧的列加入patch统计信息
								gray = src.at<uchar>(row + i, col + ksize / 2);
								patch_hist.at<int>(gray, 0) += 1;
								if (gray <= target_value)
									num_lower++;
							}

							// 根据统计信息更新target_value和num_lower
							if (num_lower < rank)
							{
								while (num_lower < rank)
								{
									target_value++;
									num_lower += patch_hist.at<int>(target_value, 0);
								}
							}
							else
							{
								while (num_lower > rank)
								{
									num_lower -= patch_hist.at<int>(target_value, 0);
									target_value--;
								}
							}

							// end of updating patch and information
						}

						// 将target_value赋值给结果像素
						dst.at<uchar>(row, col) = target_value;

					} // end of col
				}	  // end of row
			}

			int medianBlur2023Oct(std::string path)
			{
				cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
				if (image.empty())
				{
					std::cout << "Reading file error in median blur!" << std::endl;
					return -1;
				}

				cv::Mat dst;
				int ksize = 3;
				int rank = ksize * ksize / 2 + 1; // 3 x 3 / 2 + 1 = 5
				rankFilter(image, dst, ksize, rank);

				cv::Mat dst_opencv;
				cv::medianBlur(image, dst_opencv, ksize);

				cv::imshow("image", image);
				cv::imshow("median", dst);
				cv::imshow("median_opencv", dst_opencv);

				cv::waitKey(0);

				return 0;
			}

			

			cv::Mat createMat8U(const cv::Mat&src)
			{
				cv::Mat dst;
				cv::normalize(src, dst, 0, 255, cv::NORM_MINMAX);
				dst.convertTo(dst, CV_8UC1);
				return dst;
			}

			int sharpeningImage1(std::string path)
			{
				cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
				if (image.empty())
				{
					std::cout << "Reading file error in sharpening!" << std::endl;
					return -1;
				}


				cv::Mat kernel_col = (cv::Mat_<float>(3, 3) << -1, 0, 1, -1, 0, 1, -1, 0, 1);
				std::cout << kernel_col << std::endl;

				cv::Mat kernel_row = (cv::Mat_<float>(3, 3) << -1, -1, -1, 0, 0, 0, 1, 1, 1);
				std::cout << kernel_row << std::endl;

				cv::Mat grad_x;
				calcConv2D(image, grad_x,  kernel_col);
				cv::Mat grad_x8u = createMat8U(grad_x);
				cv::imshow("grad_x", grad_x8u);

				cv::Mat grad_x_abs;
				cv::convertScaleAbs(grad_x, grad_x_abs);
				cv::Mat grad_x_abs8u = createMat8U(grad_x_abs);
				cv::imshow("grad_x_abs", grad_x_abs8u);

				cv::Mat grad_y;
				calcConv2D(image, grad_y,  kernel_row);
				cv::Mat grad_y8u = createMat8U(grad_y);
				cv::imshow("grad_y", grad_y8u);

				cv::Mat grad_y_abs;
				cv::convertScaleAbs(grad_y, grad_y_abs);
				cv::Mat grad_y_abs8u = createMat8U(grad_y_abs);
				cv::imshow("grad_y_abs", grad_y_abs8u);



				cv::imshow("image", image);
				cv::waitKey(0);
				return 0;
			}

			int sharpeningImage(std::string path)
			{

				cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
				if (image.empty())
				{
					std::cout << "Reading file error in sharpening!" << std::endl;
					return -1;
				}

				cv::Mat kernel_col = (cv::Mat_<float>(3, 3) << -1, 0, 1, -1, 0, 1, -1, 0, 1);
				cv::Mat kernel_row = (cv::Mat_<float>(3, 3) << -1, -1, -1, 0, 0, 0, 1, 1, 1);
				std::cout << kernel_col << std::endl;
				std::cout << kernel_row << std::endl;

				cv::Mat dst_col, dst_row, sharp_col, sharp_row;

				calcConv2D(image, dst_col, kernel_col);
				cv::convertScaleAbs(dst_col, sharp_col);

				cv::normalize(sharp_col, sharp_col, 0, 255, cv::NORM_MINMAX);
				sharp_col.convertTo(sharp_col, CV_8UC1);
				cv::imshow("sharp_col", sharp_col);

				cv::normalize(dst_col, dst_col, 0, 255, cv::NORM_MINMAX);
				dst_col.convertTo(dst_col, CV_8UC1);
				cv::imshow("sharp_col", dst_col);

				calcConv2D(image, dst_row, kernel_row);

				cv::convertScaleAbs(dst_row, sharp_row);

				cv::normalize(sharp_row, sharp_row, 0, 255, cv::NORM_MINMAX);
				sharp_row.convertTo(sharp_row, CV_8UC1);
				cv::imshow("row_abs", sharp_row);

				cv::normalize(dst_row, dst_row, 0, 255, cv::NORM_MINMAX);
				dst_row.convertTo(dst_row, CV_8UC1);
				cv::imshow("sharp_row", dst_row);

				cv::waitKey(0);

				return 0;
			}

			

			float getDefaultSigma(int ksize)
			{
				return 0.3 * ((ksize - 1) * 0.5 - 1) + 0.8;
			}

			// type of kernel is CV_32FC1
			// type of dst_image is CV_32FC1 too
			void conv2D(const cv::Mat &image, cv::Mat &dst_image, const cv::Mat &kernel)
			{
				int k_size = kernel.cols; // for example kernel size is 5 by 5

				for (int row = k_size / 2; row < image.rows - k_size / 2; row++)
				{
					for (int col = k_size / 2; col < image.cols - k_size / 2; col++)
					{

						int center_row = row;
						int center_col = col;
						// compute convolution for pixel(row, col)
						float pixel_conv_val = 0;
						for (int offset_row = -k_size / 2; offset_row <= k_size / 2; offset_row++)
						{
							for (int offset_col = -k_size / 2; offset_col <= k_size / 2; offset_col++)
							{

								//-4 -> 0
								float pixel_gray = image.at<uchar>(center_row + offset_row, center_col + offset_col);
								int kernel_row = offset_row + k_size / 2; // 0->4
								int kernel_col = offset_col + k_size / 2;
								float kernel_element = kernel.at<float>(kernel_row, kernel_col);

								pixel_conv_val += pixel_gray * kernel_element; // is not necesssary to be in the range of 0-255
							}
						}

						dst_image.at<float>(center_row, center_col) = pixel_conv_val;
					}
				}
			}

			// int blurUsingConv2D(std::string image_path)
			// {
			// 	cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);

			// 	// if read failed, return -1;

			// 	cv::imshow("image", image);
			// 	// cv::Mat kernel = cv::Mat::zeros(3,3, CV_32FC1);
			// 	const int k_size = 9;
			// 	cv::Mat kernel = cv::Mat::ones(k_size, k_size, CV_32FC1);
			// 	kernel = kernel / (float)(k_size * k_size);

			// 	// cv::Mat kernel_average_blur = (cv::Mat_<float>(3, 3) <<
			// 	// 						  1.0/9, 1.0/9, 1.0/9,
			// 	// 						  1.0/9, 1.0/9, 1.0/9,
			// 	// 						  1.0/9, 1.0/9, 1.0/9);

			// 	cv::Mat dst_image(image.size(), CV_32FC1);

			// 	conv2D(image, dst_image, kernel);

			// 	cv::normalize(dst_image, dst_image, 0, 255, cv::NORM_MINMAX);
			// 	dst_image.convertTo(dst_image, CV_8U);

			// 	cv::imshow("blur-image", dst_image);

			// 	cv::waitKey(0);

			// 	return 0;
			// }

			void conv1D(const cv::Mat &image, cv::Mat &dst_image, float *kernel,
						int ksize, bool conv_in_horizontal)
			{
				if (conv_in_horizontal)
				{
					// convolution in the horizontal
					for (int row = 0; row < image.rows; row++)
					{
						for (int col = ksize / 2; col < image.cols - ksize / 2; col++)
						{
							float conv_val = 0;
							for (int offset = -ksize / 2; offset <= ksize / 2; offset++)
							{
								float pixel_gray = image.at<float>(row, col + offset);
								float kernel_elem = kernel[offset + ksize / 2];

								conv_val += pixel_gray * kernel_elem;
							}

							dst_image.at<float>(row, col) = conv_val;
						}
					}
				}
				else
				{
					// convolution in the vertical
					for (int row = ksize / 2; row < image.rows - ksize / 2; row++)
					{
						for (int col = 0; col < image.cols; col++)
						{
							float conv_val = 0;
							for (int offset = -ksize / 2; offset <= ksize / 2; offset++)
							{
								float pixel_gray = image.at<float>(row + offset, col);
								float kernel_elem = kernel[offset + ksize / 2];

								conv_val += pixel_gray * kernel_elem;
							}

							dst_image.at<float>(row, col) = conv_val;
						}
					}
				}
			}

			// int blurUsingConv1D(std::string image_path)
			// {

			// 	cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);

			// 	// if read failed, return -1;

			// 	cv::imshow("image", image);
			// 	cv::Mat image_float;
			// 	image.convertTo(image_float, CV_32FC1);

			// 	float kernel[5] = {1.0 / 16, 4.0 / 16, 6.0 / 16, 4.0 / 16, 1.0 / 16};

			// 	cv::Mat dst_image(image.size(), CV_32FC1);

			// 	conv1D(image_float, dst_image, kernel, 5);
			// 	conv1D(dst_image, dst_image, kernel, 5, false);

			// 	cv::normalize(dst_image, dst_image, 0, 255, cv::NORM_MINMAX);
			// 	dst_image.convertTo(dst_image, CV_8U);

			// 	cv::imshow("blur-image", dst_image);

			// 	cv::waitKey(0);

			// 	return 0;
			// }

			void rankOrderFilter2D_dummy(const cv::Mat &input, cv::Mat &output, int ksize, int order)
			{

				for (int row = ksize / 2; row < input.rows - ksize / 2; row++) ////rows///////////
				{
					for (int col = ksize / 2; col < input.cols - ksize / 2; col++)
					{
						std::vector<int> pixels;
						for (int off_row = -ksize / 2; off_row <= ksize / 2; off_row++)
						{
							for (int off_col = -ksize / 2; off_col <= ksize / 2; off_col++)
							{
								int gray = input.at<uchar>(row + off_row, col + off_col);
								pixels.push_back(gray);
							}
						}

						std::sort(pixels.begin(), pixels.end());
						output.at<uchar>(row, col) = pixels[order - 1];
					}
				}
			}

			void rankOrderFilter2D_speedUp1(const cv::Mat &input, cv::Mat &output, int ksize, int order)
			{
				const int hist_size = 256;
				for (int row = ksize / 2; row < input.rows - ksize / 2; row++)
				{
					int hist[hist_size] = {0};
					int median_val;
					int count_lower;

					for (int col = ksize / 2; col < input.cols - ksize / 2; col++)
					{
						if (col == ksize / 2)
						{
							// get all pixels' gray for current window
							std::vector<int> pixels;
							for (int offset_row = -ksize / 2; offset_row <= ksize / 2; offset_row++)
							{
								for (int offset_col = -ksize / 2; offset_col <= ksize / 2; offset_col++)
								{

									uchar gray = input.at<uchar>(row + offset_row, col + offset_col);
									hist[gray]++;
									pixels.push_back(gray);
								}
							}

							// sort the pixels based on the gray
							std::sort(pixels.begin(), pixels.end());

							median_val = pixels[order - 1];

							count_lower = 0;

							for (int i = 0; i < hist_size; i++)
							{
								if (i <= median_val)
									count_lower += hist[i];
							}
						}
						else
						{
							for (int off_row = -ksize / 2; off_row <= ksize / 2; off_row++)
							{
								int gray_left = input.at<uchar>(row + off_row, col - 2);
								hist[gray_left] = hist[gray_left] - 1;

								if (gray_left <= median_val)
									count_lower -= 1;

								int gray_right = input.at<uchar>(row + off_row, col + 1);
								hist[gray_right] += 1;
								if (gray_right <= median_val)
									count_lower++;
							}

							if (count_lower > order)
							{
								while (count_lower > order)
								{
									count_lower -= hist[median_val];
									median_val = median_val - 1;
								}
							}
							else
							{
								while (count_lower < order)
								{
									median_val++;
									count_lower += hist[median_val];
								}
							}
						}

						output.at<uchar>(row, col) = median_val;
					}
				}
			}

			void rankOrderFilter2D_speedUp(const cv::Mat &input, cv::Mat &output, int ksize, int order)
			{
				const int hist_size = 256;
				if (order < 0)
					order = (ksize * ksize) / 2 + 1;

				for (int row = ksize / 2; row < input.rows - ksize / 2; row++)
				{
					// create local histogram for the current window
					int hist[hist_size] = {0};

					// target-gray of the current window
					uchar target_gray = -1;

					// number of the pixels whose gray is less than target-gray
					int count_lower;

					// scan all candidate windows
					for (int col = ksize / 2; col < input.cols - ksize / 2; col++)
					{
						// first window
						if (col == ksize / 2)
						{

							// get all pixels' gray for current window
							std::vector<int> pixels;
							for (int offset_row = -ksize / 2; offset_row <= ksize / 2; offset_row++)
							{
								for (int offset_col = -ksize / 2; offset_col <= ksize / 2; offset_col++)
								{

									uchar gray = input.at<uchar>(row + offset_row, col + offset_col);
									hist[gray]++;
									pixels.push_back(gray);
								}
							}

							// sort the pixels based on the gray
							std::sort(pixels.begin(), pixels.end());

							// set the ORDERth pixel'gray as the target gray
							target_gray = pixels.at(order - 1);

							// compute count_lower
							count_lower = 0;
							for (int i = 0; i < pixels.size(); i++)
							{
								if (pixels[i] <= target_gray)
									count_lower++;
							}
						}
						else // right window
						{
							// update histogram and count-lower for current window
							for (int offset_row = -ksize / 2; offset_row <= ksize / 2; offset_row++)
							{
								// remove the left column from the previous window
								uchar gray_left = input.at<uchar>(row + offset_row, col - 2);

								// update the local histogram
								hist[gray_left] -= 1;

								// update the count-lower
								if (gray_left <= target_gray)
									count_lower -= 1;

								// add the right column to the previous window
								uchar gray_right = input.at<uchar>(row + offset_row, col + 1);

								// update the local histogram
								hist[gray_right] += 1;

								// update the count-lower
								if (gray_right <= target_gray)
									count_lower++;
							}

							if (count_lower > order) // the target-gray is too high
							{
								// decrease the target-gray until the count_lower <= order
								while (count_lower > order)
								{
									count_lower -= hist[target_gray];
									target_gray--;
								}
							}
							else // the target-gray is too low
							{
								// increase the target-gray until the count_lower >= order
								while (count_lower < order)
								{
									target_gray++;
									count_lower += hist[target_gray];
								}
							}
						}

						output.at<uchar>(row, col) = target_gray;
					}
				}
			}

			void rankOrderFilter2D_dummy1(
				const cv::Mat &input, cv::Mat &output, int ksize, int th)
			{
				for (int row = ksize / 2; row < input.rows - ksize / 2; row++)
				{
					for (int col = ksize / 2; col < input.cols - ksize / 2; col++)
					{
						std::vector<int> pixels;

						for (int off_row = -ksize / 2; off_row <= ksize / 2; off_row++)
						{
							for (int off_col = -ksize / 2; off_col <= ksize / 2; off_col++)
							{
								int gray = input.at<uchar>(row + off_row, col + off_col);
								pixels.push_back(gray);
							}
						}

						std::sort(pixels.begin(), pixels.end());
						output.at<uchar>(row, col) = pixels[th - 1];
					}
				}
			}

			int medianBlurEx(std::string path)
			{
				using namespace std;
				using namespace cv;

				Mat image = imread(path, cv::IMREAD_GRAYSCALE);
				if (image.empty())
				{
					std::cout << "Reading file failed!" << std::endl;
					return -1;
				}
				// resize(image, image, image.size() * 3);
				cv::imshow("Input Image", image);
				cv::waitKey(0);

				const int mask_size = 3;
				int th = mask_size * mask_size / 2 + 1; // 5

				double start, stop, duration; // record the time cost

				// compare to opencv
				Mat dst_image_opencv;
				start = static_cast<double>(getTickCount()); //

				cv::medianBlur(image, dst_image_opencv, mask_size);

				stop = static_cast<double>(getTickCount());				  //
				duration = ((double)(stop - start)) / getTickFrequency(); //
				cout << "opencv: " << duration << endl;

				cv::imshow("MedianOpenCV", dst_image_opencv);
				waitKey(0);

				// dummy way===========
				start = static_cast<double>(getTickCount()); //

				Mat dst_image = image.clone();
				rankOrderFilter2D_dummy1(image, dst_image, mask_size, th);

				stop = static_cast<double>(getTickCount());				  //
				duration = ((double)(stop - start)) / getTickFrequency(); //
				cout << "ours: " << duration << endl;

				imshow("median", dst_image);
				waitKey(0);

				// smart way==================
				start = static_cast<double>(getTickCount()); //
				Mat dst_image_speed_up = image.clone();

				rankOrderFilter2D_speedUp1(image, dst_image_speed_up, mask_size, th);

				stop = static_cast<double>(getTickCount());				  //
				duration = ((double)(stop - start)) / getTickFrequency(); //
				cout << "ours speed up: " << duration << endl;

				imshow("MedianSpeedUp", dst_image_speed_up);
				waitKey(0);

				return 0;
			}

			int medianBlur(std::string path)
			{
				const int ksize = 3;
				cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
				cv::Mat dst_image = image.clone();

				const int hist_size = 256;

				int th = (ksize * ksize) / 2 + 1;

				for (int row = ksize / 2; row < image.rows - ksize / 2; row++)
				{
					int hist[hist_size] = {0};
					uchar med_val = -1;

					int count_lower;

					for (int col = ksize / 2; col < image.cols - ksize / 2; col++)
					{

						if (col == ksize / 2) // first window in current row
						{
							std::vector<int> all_pixels;
							for (int offset_row = -ksize / 2; offset_row <= ksize / 2; offset_row++)
							{
								for (int offset_col = -ksize / 2; offset_col <= ksize / 2; offset_col++)
								{
									int gray = image.at<uchar>(row + offset_row, col + offset_col);
									hist[gray]++;
									all_pixels.push_back(gray);
								}
							}
							std::sort(all_pixels.begin(), all_pixels.end());
							med_val = all_pixels.at((ksize * ksize) / 2);

							count_lower = 0;
							for (int i = 0; i < all_pixels.size(); i++)
							{
								if (all_pixels[i] <= med_val)
									count_lower++;
							}
							// count_lower = (ksize * ksize) / 2 + 1; // th
						}
						else // other windows in the right of the first window for the current row
						{
							for (int offset_row = -ksize / 2; offset_row <= ksize / 2; offset_row++)
							{
								// remove left column
								int gray_left = image.at<uchar>(row + offset_row, col - 2);
								hist[gray_left] -= 1;

								if (gray_left <= med_val)
									count_lower -= 1;

								// add right column
								int gray_right = image.at<uchar>(row + offset_row, col + 1);
								hist[gray_right] += 1;

								if (gray_right <= med_val)
									count_lower += 1;
							}

							if (count_lower > th) // if the median value is too high
							{
								while (count_lower > th)
								{
									count_lower -= hist[med_val];
									med_val -= 1;
								}
							}
							else // if the median value is too low
							{
								while (count_lower < th)
								{
									med_val += 1;
									count_lower += hist[med_val];
								}
							}
						}

						dst_image.at<uchar>(row, col) = med_val;
					}
				}

				cv::imshow("image", image);
				cv::imshow("medianBlured Image", dst_image);
				cv::waitKey(0);
				return 0;
			}

			int medianBlur3Ways(std::string path)
			{
				using namespace std;
				using namespace cv;

				Mat image = imread(path, cv::IMREAD_GRAYSCALE);
				if (image.empty())
				{
					std::cout << "Reading file failed!" << std::endl;
					return -1;
				}
				// resize(image, image, image.size() * 3);
				cv::imshow("Input Image", image);
				cv::waitKey(0);

				const int mask_size = 3;
				int th = mask_size * mask_size / 2 + 1; // 5

				double start, stop, duration; // record the time cost

				// compare to opencv
				Mat dst_image_opencv;
				start = static_cast<double>(getTickCount()); //

				cv::medianBlur(image, dst_image_opencv, mask_size);

				stop = static_cast<double>(getTickCount());				  //
				duration = ((double)(stop - start)) / getTickFrequency(); //
				cout << "opencv: " << duration << endl;

				cv::imshow("MedianOpenCV", dst_image_opencv);
				waitKey(0);

				// dummy way===========
				start = static_cast<double>(getTickCount()); //

				Mat dst_image = image.clone();
				uchar pixelsLocal[mask_size * mask_size];
				for (int i = 1; i < image.rows - 1; i++) ////rows///////////
				{
					for (int j = 1; j < image.cols - 1; j++)
					{
						for (int m = -1; m <= 1; m++)
						{
							for (int n = -1; n <= 1; n++)
							{
								pixelsLocal[(m + 1) * mask_size + (n + 1)] = image.at<uchar>(i + m, j + n);
							}
						}
						std::sort(pixelsLocal, pixelsLocal + mask_size * mask_size);
						dst_image.at<uchar>(i, j) = pixelsLocal[mask_size * mask_size / 2];
					}
				}

				stop = static_cast<double>(getTickCount());				  //
				duration = ((double)(stop - start)) / getTickFrequency(); //
				cout << "ours: " << duration << endl;

				imshow("median", dst_image);
				waitKey(0);

				// smart way==================
				start = static_cast<double>(getTickCount()); //
				Mat dst_image_speed_up = image.clone();
				for (int i = 1; i < image.rows - 1; i++) ////rows///////////
				{
					int count_lower;
					uchar median_gray;
					int histLocal[256] = {0};
					for (int j = 1; j < image.cols - 1; j++) /////cols///////////
					{
						if (j == 1) /// first window in current row
						{
							// init local histogram
							for (int r = -1; r <= 1; r++)
							{
								for (int c = -1; c <= 1; c++)
								{
									int gray = image.at<uchar>(i + r, j + c);
									histLocal[gray]++;
								}
							}
							// init median value and number of pixels whose gray is weaker than the threshold
							count_lower = 0;
							median_gray = 0;
							for (int gray = 0; gray < 256; gray++)
							{
								median_gray = gray;
								count_lower += histLocal[gray];
								if (count_lower > th)
								{
									count_lower -= histLocal[gray];
									median_gray--;
									break;
								}
							}
							/////////////////////////////////////////////////
						}
						else // the other windows than the first one in current row
						{
							// update the local histogram
							for (int r = -1; r <= 1; r++) // remove information of the old col
							{
								int gray = image.at<uchar>(i + r, j - 2); // remove old
								histLocal[gray]--;
								if (gray <= median_gray)
									count_lower--;
							}
							for (int r = -1; r <= 1; r++) // add information of the new col
							{
								int gray = image.at<uchar>(i + r, j + 1); // add new
								histLocal[gray]++;
								if (gray <= median_gray)
									count_lower++;
							}
							// update mediean gray and number of pixels whose gray is weaker than the median
							if (count_lower > th)
							{
								while (count_lower > th)
								{
									count_lower -= histLocal[median_gray];
									median_gray--;
								}
							}
							else
							{
								while (count_lower < th)
								{
									median_gray++;
									count_lower += histLocal[median_gray];
								}
							}
							// dstImage.at<uchar>(i, j) = medGray;

						} /////////////////////////////////////////////////////////////

						dst_image_speed_up.at<uchar>(i, j) = median_gray;
					}
				}
				stop = static_cast<double>(getTickCount());				  //
				duration = ((double)(stop - start)) / getTickFrequency(); //
				cout << "ours speed up: " << duration << endl;

				imshow("MedianSpeedUp", dst_image_speed_up);
				waitKey(0);

				return 0;
			}

			// void conv2D(const cv::Mat &image, cv::Mat &dst_image, const cv::Mat &kernel, int ker_size)
			// {
			// 	for (int i = ker_size / 2; i < image.rows - ker_size / 2; i++)
			// 	{
			// 		for (int j = ker_size / 2; j < image.cols - ker_size / 2; j++)
			// 		{
			// 			float value = 0;
			// 			for (int m = -ker_size / 2; m <= ker_size / 2; m++)
			// 			{
			// 				for (int n = -ker_size / 2; n <= ker_size / 2; n++)
			// 				{
			// 					float gray = image.at<uchar>(i + m, j + n);
			// 					float kernel_element = kernel.at<float>(m + ker_size / 2, n + ker_size / 2);

			// 					value += gray * kernel_element;
			// 				}
			// 			}

			// 			dst_image.at<float>(i, j) = value;
			// 		}
			// 	}
			// }

			// void conv1D(const cv::Mat &image, cv::Mat &dst_image, float *kernel, int ker_size, bool row_vector_flag = true)
			// {
			// 	if (row_vector_flag) // kernel is a row vector (1 × kernel_size), like (1,2,3)
			// 	{
			// 		for (int i = 0; i < image.rows; i++)
			// 		{
			// 			for (int j = ker_size / 2; j < image.cols - ker_size / 2; j++)
			// 			{
			// 				float value = 0;

			// 				for (int n = -ker_size / 2; n <= ker_size / 2; n++)
			// 				{
			// 					float gray = image.at<float>(i, j + n);
			// 					float kernel_element = kernel[n + ker_size / 2];

			// 					value += gray * kernel_element;
			// 				}

			// 				dst_image.at<float>(i, j) = value;
			// 			}
			// 		}
			// 	}
			// 	else // kernel is a column vector (kernel_size × 1), like (1,2,3)'
			// 	{
			// 		for (int i = ker_size / 2; i < image.rows - ker_size / 2; i++)
			// 		{
			// 			for (int j = 0; j < image.cols; j++)
			// 			{
			// 				float value = 0;

			// 				for (int m = -ker_size / 2; m <= ker_size / 2; m++)
			// 				{
			// 					float gray = image.at<float>(i + m, j);
			// 					float kernel_element = kernel[m + ker_size / 2];

			// 					value += gray * kernel_element;
			// 				}

			// 				dst_image.at<float>(i, j) = value;
			// 			}
			// 		}
			// 	}
			// }

			int computeGradient(std::string path)
			{
				using namespace std;
				using namespace cv;

				Mat image = imread(path, cv::IMREAD_GRAYSCALE);
				if (image.empty())
				{
					std::cout << "Reading file failed!" << std::endl;
					return -1;
				}
				cv::imshow("Image", image);
				cv::waitKey(0);

				Mat dst_image = cv::Mat::zeros(image.size(), CV_32FC1);

				Mat kernel_laplace = (Mat_<float>(3, 3) << 0, -1, 0,
									  -1, 4, -1,
									  0, -1, 0);

				Mat kernel_prewitt_horizontal = (Mat_<float>(3, 3) << -1, 0, 1,
												 -1, 0, 1,
												 -1, 0, 1);

				Mat kernel_prewitt_vertical = (Mat_<float>(3, 3) << -1, -1, -1,
											   0, 0, 0,
											   1, 1, 1);

				conv2D(image, dst_image, kernel_laplace);
				cv::convertScaleAbs(dst_image, dst_image);
				// double min_val = 0, max_val = 0;
				// minMaxLoc(dstImage, &min_val, &max_val);

				cv::normalize(dst_image, dst_image, 0, 255, cv::NORM_MINMAX);
				dst_image.convertTo(dst_image, CV_8U);
				cv::imshow("Prewitt", dst_image);

				/*Mat dstImageOpenCV;
				filter2D(image, dstImageOpenCV, image.type(), kern);
				double min_val1 = 0, max_val1 = 0; //
				minMaxLoc(dstImageOpenCV, &min_val1, &max_val1);

				imshow("PrewittOpenCV", dstImageOpenCV);
				//*/

				cv::waitKey(0);

				return 0;
			}

			int blurUsingConv2D(std::string path)
			{
				using namespace std;
				using namespace cv;

				Mat image = imread(path, cv::IMREAD_GRAYSCALE);
				if (image.empty())
				{
					std::cout << "Reading file failed!" << std::endl;
					return -1;
				}
				cv::imshow("Image", image);
				cv::waitKey(0);

				Mat dst_image = cv::Mat::zeros(image.size(), CV_32FC1);

				Mat kernel_col_vector = (Mat_<float>(5, 1) << 1.0 / 16, 4.0 / 16, 6.0 / 16, 4.0 / 16, 1.0 / 16);
				Mat kernel_row_vector = kernel_col_vector.t(); // tranpose of kernel1 is a Mat of 1 row 1 by 5 cols
				Mat kernel = kernel_col_vector * kernel_row_vector;

				double start = static_cast<double>(cv::getTickCount());

				conv2D(image, dst_image, kernel);

				double stop = static_cast<double>(getTickCount());
				double duration = ((double)(stop - start)) / getTickFrequency();
				cout << "blur using conv2D: " << duration << endl;

				// cv::convertScaleAbs(dstImage, dstImage);
				//  double min_val = 0, max_val = 0;
				//  cv::minMaxLoc(dst_image, &min_val, &max_val);

				cv::normalize(dst_image, dst_image, 0, 255, cv::NORM_MINMAX);
				dst_image.convertTo(dst_image, CV_8U);

				cv::imshow("Blur", dst_image);

				/*Mat dstImageOpenCV;
				filter2D(image, dstImageOpenCV, image.type(), kern);
				double min_val1 = 0, max_val1 = 0;
				minMaxLoc(dstImageOpenCV, &min_val1, &max_val1);

				imshow("Conv2DOpenCV", dstImageOpenCV);
				//*/

				cv::waitKey(0);

				return 0;
			}
			int blurUsingConv1D(std::string path)
			{
				using namespace std;
				using namespace cv;

				Mat image = imread(path, cv::IMREAD_GRAYSCALE);
				if (image.empty())
				{
					std::cout << "Reading file failed!" << std::endl;
					return -1;
				}
				cv::imshow("Image", image);
				cv::waitKey(0);

				image.convertTo(image, CV_32F);

				Mat dst_image = cv::Mat::zeros(image.size(), CV_32FC1);

				float kernel[] = {1.0 / 16, 4.0 / 16, 6.0 / 16, 4.0 / 16, 1.0 / 16};

				double start, stop, duration; //
				start = static_cast<double>(cv::getTickCount());

				conv1D(image, dst_image, kernel, 5, true);
				conv1D(dst_image, dst_image, kernel, 5, false);

				stop = static_cast<double>(cv::getTickCount());
				duration = ((double)(stop - start)) / cv::getTickFrequency();
				cout << "blur using conv1D: " << duration << endl;

				// cv::convertScaleAbs(dstImage, dstImage);
				/// double min_val = 0, max_val = 0;
				// minMaxLoc(dstImage, &min_val, &max_val);

				cv::normalize(dst_image, dst_image, 0, 255, cv::NORM_MINMAX);
				dst_image.convertTo(dst_image, CV_8U);
				cv::imshow("Blur", dst_image);

				/*Mat dstImageOpenCV;
				filter2D(image, dstImageOpenCV, image.type(), kern);
				double min_val1 = 0, max_val1 = 0;
				minMaxLoc(dstImageOpenCV, &min_val1, &max_val1);

				imshow("PrewittOpenCV", dstImageOpenCV);
				//*/

				waitKey(0);

				return 0;
			}

			cv::Mat getGaussianKernel2D(int size, double sigma)
			{
				using namespace cv;
				using namespace std;

				Mat kernelX = getGaussianKernel(size, sigma);
				// cvrsg::common::Utilities::PrintMatrix<double>(kernelX);
				cout << kernelX << endl
					 << endl
					 << endl;

				Mat kernelY = getGaussianKernel(size, sigma);
				// Mat kernelY = kernelX.clone();
				// cvrsg::common::Utilities::PrintMatrix<double>(kernelY.t());
				cout << kernelY.t() << endl
					 << endl
					 << endl;

				Mat kernelG = kernelX * kernelY.t();

				// cvrsg::common::Utilities::PrintMatrix<double>(kernelG);
				cout << kernelG << endl
					 << endl
					 << endl;
				return kernelG;
			}

			inline int getKsizeFromSigma(float sigma)
			{
				// sigma = 0.3 *( (ksize-1) * 0.5 - 1) + 0.8
				//( (sigma - 0.8)/0.3 + 1) *2 + 1 =  ksize

				int ksize = ((sigma - 0.8) / 0.3 + 1) * 2 + 1;
				ksize = ksize / 2 * 2 + 1;

				return ksize;
			}
		}
	}
}
