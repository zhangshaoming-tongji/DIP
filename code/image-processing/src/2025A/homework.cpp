#include <opencv2/opencv.hpp>
#include <string>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include "Drawing.h"
#include <vector>

#include "2025A/homework.h"

namespace DIPAutumn2025
{
    namespace private_trans_conv
    {
        cv::Mat calcAccHist(const cv::Mat &image)
        {
            const int gray_rank = 256;
            cv::Mat hist = cv::Mat::zeros(gray_rank, 1, CV_32FC1); // 256x1 matrix initialized to zeros

            for (int row = 0; row < image.rows; row++)
            {
                for (int col = 0; col < image.cols; col++)
                {
                    int gray = image.at<uchar>(row, col);
                    hist.at<float>(gray, 0) += 1;
                }
            }
            std::cout << "hist: " << hist.t() << std::endl;

            cv::Mat acc_hist = cv::Mat::zeros(gray_rank, 1, CV_32FC1);
            for (int gray = 0; gray < gray_rank; gray++)
            {
                if (gray == 0)
                    acc_hist.at<float>(gray, 0) = hist.at<float>(gray, 0);
                else
                    acc_hist.at<float>(gray, 0) =
                        acc_hist.at<float>(gray - 1, 0) + hist.at<float>(gray, 0);
            }
            std::cout << "acc hist: " << acc_hist.t() << std::endl;

            return acc_hist;
        }

        int conv2dImpSquareKernel(const cv::Mat &src, cv::Mat &dst, const cv::Mat &kernel)
        {
            if (!(src.depth() == CV_32F || src.depth() == CV_8U))
            {
                std::cout << "Error: Only Float32 and Unsigned Char are supported!" << std::endl;
                return -1;
            }

            if (dst.empty())
                dst = cv::Mat(src.size(), CV_32FC1);

            int ksize = kernel.cols / 2;

            for (int row = 0; row < src.rows; row++)
            {
                for (int col = 0; col < src.cols; col++)
                {
                    float conv_result = 0;
                    for (int i = -ksize; i <= ksize; i++) // row of kernel
                    {
                        for (int j = -ksize; j <= ksize; j++) // col of kernel
                        {
                            int pixel_row = row + i;
                            int pixel_col = col + j;

                            float pixel_value = 0;

                            if (pixel_col < 0 || pixel_col > src.cols - 1 ||
                                pixel_row < 0 || pixel_row > src.rows - 1)
                                pixel_value = 0;
                            else
                            {
                                if (src.depth() == CV_32F)
                                    pixel_value = src.at<float>(pixel_row, pixel_col);
                                if (src.depth() == CV_8U)
                                    pixel_value = src.at<uchar>(pixel_row, pixel_col);
                            }

                            float kernel_value = kernel.at<float>(i + ksize, j + ksize);

                            conv_result += pixel_value * kernel_value;
                        }
                    }
                    dst.at<float>(row, col) = conv_result;
                }
            }

            return 0;
        }

        cv::Mat loadKernel(std::string txt_path)
        {
            int rows, cols;

            FILE *file = fopen(txt_path.c_str(), "r+");
            fscanf(file, "%d,%d\n", &rows, &cols);

            cv::Mat kernel = cv::Mat::zeros(rows, cols, CV_32FC1);

            for (int row = 0; row < rows; row++)
            {
                for (int col = 0; col < cols - 1; col++)
                {
                    float temp;
                    fscanf(file, "%f,", &temp);
                    kernel.at<float>(row, col) = temp;
                }
                float temp;
                fscanf(file, "%f\n", &temp);
                kernel.at<float>(row, cols - 1) = temp;
            }

            fclose(file);

            // float sum = cv::sum(kernel)[0];
            // kernel /= sum;

            return kernel;
        }

        int conv2dImpRectangleKernel(const cv::Mat &src, cv::Mat &dst, const cv::Mat &kernel)
        {
            if (!(src.depth() == CV_32F || src.depth() == CV_8U))
            {
                std::cout << "Error: Only Float32 and Unsigned Char are supported!" << std::endl;
                return -1;
            }

            if (dst.empty())
                dst = cv::Mat(src.size(), CV_32FC1);

            int ksize_row = kernel.rows / 2;
            int ksize_col = kernel.cols / 2;

            for (int row = 0; row < src.rows; row++)
            {
                for (int col = 0; col < src.cols; col++)
                {
                    float conv_result = 0;
                    for (int row_off = -ksize_row; row_off <= ksize_row; row_off++) // row of kernel
                    {
                        for (int col_off = -ksize_col; col_off <= ksize_col; col_off++) // col of kernel
                        {
                            int pixel_row = row + row_off;
                            int pixel_col = col + col_off;

                            float pixel_value = 0;

                            if (pixel_col < 0 || pixel_col > src.cols - 1 ||
                                pixel_row < 0 || pixel_row > src.rows - 1)
                                pixel_value = 0;
                            else
                            {
                                if (src.depth() == CV_32F)
                                    pixel_value = src.at<float>(pixel_row, pixel_col);
                                else if (src.depth() == CV_8U)
                                    pixel_value = src.at<uchar>(pixel_row, pixel_col);
                            }

                            float kernel_value =
                                kernel.at<float>(row_off + ksize_row, col_off + ksize_col);

                            conv_result += pixel_value * kernel_value;
                        }
                    }
                    dst.at<float>(row, col) = conv_result;
                }
            }

            return 0;
        }

    }; // namespace private


    int grayTrans(char *dir, char *filename)
    {
        cv::Mat image = cv::imread(std::string(dir) + std::string(filename), cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cerr << "Error: Could not open or find the image!" << std::endl;
            return -1;
        }
        std::cout << "Image loaded: " << filename << ", size: " << image.cols << "x" << image.rows << std::endl;

        cv::imshow("Original Image", image);
        cvrsg::common::Drawing::DrawAndShowHist(image, "Original Histogram");
        // cv::waitKey(0);

        cv::Mat acc_hist = private_trans_conv::calcAccHist(image);

        cv::Mat trans_image = cv::Mat::zeros(image.size(), CV_8UC1);
        for (int row = 0; row < image.rows; row++)
        {
            for (int col = 0; col < image.cols; col++)
            {
                int input = image.at<uchar>(row, col);

                float output = acc_hist.at<float>(input, 0) * 255.0f / (image.cols * image.rows);

                trans_image.at<uchar>(row, col) = output;
            }
        }

        cv::imshow("Transformed Image", trans_image);
        cvrsg::common::Drawing::DrawAndShowHist(trans_image, "Transformed Histogram");
        // cv::waitKey(0);

        return 0;
    }

    int geoTrans(char *dir, char *image_name, char *params_filename)
    {

        double deg, sx, sy, tx, ty;
        FILE *file = fopen((std::string(dir) + std::string(params_filename)).c_str(), "r+");
        fscanf(file, "%lf,%lf,%lf,%lf,%lf", &deg, &sx, &sy, &tx, &ty);
        fclose(file);

        float rotation = deg / 180 * CV_PI;

        cv::Mat affine_matrix = cv::Mat::zeros(2, 3, CV_64F);
        affine_matrix.at<double>(0, 0) = sx * cos(rotation);
        affine_matrix.at<double>(0, 1) = -sin(rotation);
        affine_matrix.at<double>(0, 2) = tx;
        affine_matrix.at<double>(1, 0) = sin(rotation);
        affine_matrix.at<double>(1, 1) = sy * cos(rotation);
        affine_matrix.at<double>(1, 2) = ty;

        cv::Mat image = cv::imread(std::string(dir) + std::string(image_name), cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "error when reading image!" << std::endl;
            return -1;
        }
        cv::imshow("image", image);
        cv::Mat result;
        warpAffine(image, result, affine_matrix, image.size() * 2);
        cv::imshow("affine_result", result);
        cv::waitKey(0);

        return 0;
    }

    int conv2d(char *dir, char *image_name, char *kernel_name)
    {
        cv::Mat kernel = private_trans_conv::loadKernel(std::string(dir) + std::string(kernel_name));
        std::cout << "kernel: " << std::endl
                  << kernel << std::endl;

        cv::Mat image = cv::imread(std::string(dir) + std::string(image_name), cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "error reading image!" << std::endl;
            return -1;
        }
        cv::imshow("image", image);

        cv::Mat output;

        // count time start
        double start = static_cast<double>(cv::getTickCount()); //

        private_trans_conv::conv2dImpSquareKernel(image, output, kernel);

        // count time end
        double end = static_cast<double>(cv::getTickCount());
        double duration = ((double)(end - start) / cv::getTickFrequency());
        std::cout << "Duration conv_2d: " << duration * 1000 << " ms." << std::endl;

        cv::normalize(output, output, 0, 255, cv::NORM_MINMAX);
        output.convertTo(output, CV_8U);

        cv::imshow("conv", output);

        std::string out_path = std::string(dir) + "conv2d_result.png";
        cv::imwrite(out_path, output);
        std::cout << "saved: " << out_path << std::endl;

        std::cout << "conv2d: Left-Top 5X5: (conv2d): " << std::endl
                  << output(cv::Rect(0, 0, 5, 5)) << std::endl;

        if (std::getenv("DIP_NO_WAIT"))
            cv::waitKey(1);
        else
            cv::waitKey(0);

        return 0;
    }

    int conv2dSeparated(char *dir, char *image_name, char *kernel_name)
    {
        cv::Mat kernel = private_trans_conv::loadKernel(std::string(dir) + std::string(kernel_name));

        std::cout << "kernel: " << std::endl
                  << kernel << std::endl;

        cv::Mat kernel_t = kernel.t();
        std::cout << "kernel_t: " << std::endl
                  << kernel.t() << std::endl;

        cv::Mat kernel_2d = kernel * kernel.t();
        float sum_2d = cv::sum(kernel_2d)[0];
        std::cout << "kernel_2d: " << std::endl
                  << kernel_2d << std::endl;
        std::cout << "sum(kernel_2d): " << sum_2d << std::endl;

        cv::Mat image = cv::imread(std::string(dir) + std::string(image_name), cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "error reading image!" << std::endl;
            return -1;
        }

        cv::Mat imageF;
        image.convertTo(imageF, CV_32F);

        cv::Mat output_mid, output; // = cv::Mat(image.rows, image.cols, CV_32FC1);

        // count time start
        double start = static_cast<double>(cv::getTickCount()); //

        private_trans_conv::conv2dImpRectangleKernel(imageF, output_mid, kernel);
        private_trans_conv::conv2dImpRectangleKernel(output_mid, output, kernel_t);

        // count time end
        double end = static_cast<double>(cv::getTickCount());
        double duration = ((double)(end - start) / cv::getTickFrequency());
        std::cout << "Duration conv_2d_separated: " << duration * 1000 << " ms." << std::endl;

        cv::normalize(output, output, 0, 255, cv::NORM_MINMAX);
        output.convertTo(output, CV_8U);
        cv::imshow("conv_separ", output);

        std::cout << "conv2d_separated: Left-Top 5X5: " << std::endl
                  << output(cv::Rect(0, 0, 5, 5)) << std::endl;

        cv::waitKey(0);

        return 0;
    }
}
