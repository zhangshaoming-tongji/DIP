
#include <cstdio>
#include "opencv.hpp"

#include "convolution2024A.h"

namespace DIPAutumn2024
{

    cv::Mat loadKernel_onclass(std::string kernel_path)
    {
        int rows, cols;
        FILE *file = fopen(kernel_path.c_str(), "r+");
        fscanf(file, "%d,%d\n", &rows, &cols);

        cv::Mat kernel = cv::Mat(rows, cols, CV_32F);

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

        std::cout << "kernel: " << std::endl
                  << kernel << std::endl;

        return kernel;
    }

    int conv2dImp_onclass(const cv::Mat &image, cv::Mat &output, const cv::Mat &kernel)
    {
        output = image.clone(); //(image.size(), CV_32F);

        int ksize = kernel.rows;
        for (int row = ksize / 2; row < image.rows - ksize / 2; row++)
        {
            for (int col = ksize / 2; col < image.cols - ksize / 2; col++)
            {

                float conv_val = 0;
                for (int row_offset = -ksize / 2; row_offset <= ksize / 2; row_offset++)
                {
                    for (int col_offset = -ksize / 2; col_offset <= ksize / 2; col_offset++)
                    {
                        int cur_row = row + row_offset;
                        int cur_col = col + col_offset;
                        float cur_pixel = image.at<float>(cur_row, cur_col);

                        int cur_k_row = row_offset + ksize / 2;
                        int cur_k_col = col_offset + ksize / 2;
                        float cur_kernel = kernel.at<float>(cur_k_row, cur_k_col);

                        conv_val += cur_pixel * cur_kernel;
                    }
                }

                output.at<float>(row, col) = conv_val;
            }
        }

        return 0;
    }

    int conv2d_onclass(std::string image_path, std::string kernel_path)
    {
        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "conv2d error: reading image failed!" << std::endl;
            return -1;
        }
        cv::imshow("image", image);

        cv::Mat kernel = loadKernel_onclass(kernel_path);

        cv::Mat imageF;
        image.convertTo(imageF, CV_32F);

        cv::Mat output;
        conv2dImp_onclass(imageF, output, kernel);

        cv::normalize(output, output, 0, 255, cv::NORM_MINMAX);
        output.convertTo(output, CV_8U);

        cv::imshow("conv2d", output);
        cv::waitKey(0);

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

    /// @brief convolution 2-dimension, the rows and cols of the kernel must be the same
    /// @param src
    /// @param dst
    /// @param kernel
    /// @return
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

    /// @brief 2-dimension convolution, rows and cols of kernel are not the same
    /// @param src type must be float32
    /// @param dst type is float32
    /// @param kernel column or row vector
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

    int conv2d(std::string image_path, std::string kernel_path)
    {
        cv::Mat kernel = loadKernel(kernel_path);
        std::cout << "kernel: " << std::endl
                  << kernel << std::endl;

        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "error reading image!" << std::endl;
            return -1;
        }

        cv::Mat output;

        // count time start
        double start = static_cast<double>(cv::getTickCount()); //

        conv2dImpSquareKernel(image, output, kernel);

        // count time end
        double end = static_cast<double>(cv::getTickCount());
        double duration = ((double)(end - start) / cv::getTickFrequency());
        std::cout << "Duration conv_2d: " << duration * 1000 << " ms." << std::endl;

        cv::normalize(output, output, 0, 255, cv::NORM_MINMAX);
        output.convertTo(output, CV_8U);

        cv::imshow("conv", output);

        std::cout << "conv2d: Left-Top 5X5: (conv2d): " << std::endl
                  << output(cv::Rect(0, 0, 5, 5)) << std::endl;

        cv::waitKey(0);

        return 0;
    }

    void conv2dImpRectKernel_onclass(
        const cv::Mat &input, cv::Mat &output, const cv::Mat &kernel)
    {
        int ksize_row = kernel.rows / 2;
        int ksize_col = kernel.cols / 2;

        output = input.clone();
        for (int row = ksize_row; row < input.rows - ksize_row; row++)
        {
            for (int col = ksize_col; col < input.cols - ksize_col; col++)
            {
                float conv_result = 0;
                for (int k_row = 0; k_row < kernel.rows; k_row++)
                {
                    for (int k_col = 0; k_col < kernel.cols; k_col++)
                    {
                        int image_row = row - ksize_row + k_row;
                        int image_col = col - ksize_col + k_col;

                        float pixel_val = input.at<float>(image_row,image_col);
                        float k_val = kernel.at<float>(k_row, k_col);

                        conv_result += pixel_val * k_val;    
                    }
                }

                output.at<float>(row, col) = conv_result;

            }
        }
    }

    int conv2dSeparated_onclass(std::string image_path, std::string kernel_path)
    {
        cv::Mat kernel = loadKernel(kernel_path);

        std::cout << "kernel: " << std::endl
                  << kernel << std::endl;

        std::cout << "kernel_transpose: " << std::endl
                  << kernel.t() << std::endl;

        cv::Mat kernel_2d = kernel * kernel.t();
        std::cout << "kernel_2d: " << std::endl
                  << kernel_2d << std::endl;

        float sum = cv::sum(kernel_2d)[0];
        std::cout << "kernel_2d_sum: " << sum << std::endl;

        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "conv2dsep_onclass: error when reading image" << std::endl;
            return -1;
        }
        cv::imshow("image", image);

        cv::Rect roi(2,2,3,3);

        //2d convolution
        cv::Mat imageF;
        image.convertTo(imageF, CV_32F);
        cv::Mat output_conv_2d;
        conv2dImp_onclass(imageF, output_conv_2d, kernel_2d);
        std::cout<< "conv2d top_left:" <<std::endl<<output_conv_2d(roi)<<std::endl;
        
        cv::normalize(output_conv_2d, output_conv_2d, 0, 255, cv::NORM_MINMAX);
        output_conv_2d.convertTo(output_conv_2d, CV_8U);
        cv::resize(output_conv_2d, output_conv_2d, output_conv_2d.size()*2);
        cv::imshow("conv2d", output_conv_2d);

        //1d convolution
        cv::Mat mid_output;
        cv::Mat output_conv_1d;
        
        conv2dImpRectKernel_onclass(imageF,mid_output, kernel);
        // output_conv_1d = mid_output.clone();
        // cv::normalize(output_conv_1d, output_conv_1d, 0, 255, cv::NORM_MINMAX);
        // output_conv_1d.convertTo(output_conv_1d, CV_8U);
        // cv::resize(output_conv_1d, output_conv_1d, output_conv_1d.size()*2);
        // cv::imshow("conv2d_separ_row", output_conv_1d);

        conv2dImpRectKernel_onclass(mid_output, output_conv_1d, kernel.t());
        
        std::cout<< "conv2d_sep top_left:" <<std::endl<<output_conv_1d(roi)<<std::endl;
        cv::normalize(output_conv_1d, output_conv_1d, 0, 255, cv::NORM_MINMAX);
        output_conv_1d.convertTo(output_conv_1d, CV_8U);
        cv::resize(output_conv_1d, output_conv_1d, output_conv_1d.size() * 2);
        cv::imshow("conv2d_separ", output_conv_1d);

        
        

        cv::waitKey(0);

        return 0;
    }

    int conv2dSeparated(std::string image_path, std::string kernel_path)
    {
        cv::Mat kernel = loadKernel(kernel_path);

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

        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
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

        conv2dImpRectangleKernel(imageF, output_mid, kernel);
        conv2dImpRectangleKernel(output_mid, output, kernel_t);
        // conv1dImp(imageF, output_mid, kernel);
        // conv1dImp(output_mid, output, kernel_t);

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

#define __DEBUG_MEDIAN_SORTING_FILTER 0

    std::vector<int> calcHist(const cv::Mat &input)
    {
        const int gray_rank = 256;

        std::vector<int> hist(gray_rank, 0);
        for (int row = 0; row < input.rows; row++)
        {
            for (int col = 0; col < input.cols; col++)
            {
                int gray = input.at<uchar>(row, col);
                hist[gray] += 1;
            }
        }

        return hist;
    }

    void sortPatch(const cv::Mat &image, int row, int col, int target_integral, uchar &med_val)
    {
        std::vector<uchar> patch_array; // = image(cv::Rect(col - 1, row - 1, 3, 3)).clone().reshape(0,1);
        for (int patch_row = -1; patch_row <= 1; patch_row++)
        {
            for (int patch_col = -1; patch_col <= 1; patch_col++)
            {
                patch_array.push_back(image.at<uchar>(row + patch_row, col + patch_col));
            }
        }

        std::sort(patch_array.begin(), patch_array.end());

#if __DEBUG_MEDIAN_SORTING_FILTER
        std::cout << "patch_array: " << patch_array << std::endl;
#endif

        med_val = patch_array[target_integral - 1];
    }

    void initSorting(const cv::Mat &image, int row, int target_integral,
                     std::vector<int> &patch_hist, uchar &med_val, int &integral)
    {
        cv::Mat patch_mat = image(cv::Rect(0, row - 1, 3, 3)).clone();

        patch_hist = calcHist(patch_mat);
#if __DEBUG_MEDIAN_SORTING_FILTER
        std::cout << "sub_hist: " << std::endl
                  << cv::Mat(patch_hist).t() << std::endl;
#endif
        cv::Mat patch_array = patch_mat.reshape(0, 1);
        cv::sort(patch_array, patch_array, cv::SORT_ASCENDING);

#if __DEBUG_MEDIAN_SORTING_FILTER
        std::cout << "patch_array: " << patch_array << std::endl;
#endif

        med_val = patch_array.at<uchar>(0, target_integral - 1);

        integral = 5;
        for (int i = 5; i < 9; i++)
        {
            uchar pixel = patch_array.at<uchar>(0, i);
            if (pixel <= med_val)
                integral++;
        }
#if __DEBUG_MEDIAN_SORTING_FILTER
        std::cout << "integral: " << integral << std::endl;
#endif
    }

    void updateSorting(const cv::Mat &image, int row, int col, int target_integral,
                       std::vector<int> &patch_hist, uchar &med_val, int &integral)
    {
        // remove left column and update sub_hist
        std::vector<uchar> column_changed = image(cv::Rect(col - 2, row - 1, 1, 3)).clone().reshape(0, 1);
#if __DEBUG_MEDIAN_SORTING_FILTER
        std::cout << "col_removed: " << std::endl
                  << cv::Mat(column_changed) << std::endl;
#endif
        for (int patch_row = 0; patch_row < 3; patch_row++)
        {
            int gray = column_changed[patch_row];
            patch_hist[gray] -= 1;
            if (gray <= med_val)
                integral--;
        }

        // add new column and update sub_hist
        column_changed = image(cv::Rect(col + 1, row - 1, 1, 3)).clone().reshape(0, 1);
#if __DEBUG_MEDIAN_SORTING_FILTER
        std::cout << "col_added: " << std::endl
                  << cv::Mat(column_changed) << std::endl;
#endif
        for (int patch_row = 0; patch_row < 3; patch_row++)
        {
            int gray = column_changed[patch_row];
            patch_hist[gray] += 1;
            if (gray <= med_val)
                integral++;
        }

        // update median value
        if (integral > target_integral)
        {
            do
            {
                int med_pix_count = patch_hist[med_val];
                if (integral - med_pix_count >= target_integral)
                {
                    integral -= med_pix_count;
                    med_val--;
                }
                else // 只保留右侧，如果已经到了左侧，则不再执行med_val--操作
                    break;

            } while (integral > target_integral);
        }
        else if (integral < target_integral)
        {
            do
            {
                med_val++;
                integral += patch_hist[med_val];
            } while (integral < target_integral);
        }
    }

    void show_debug(const cv::Mat &image, int row, int col)
    {
        cv::Mat debug_mat = image(cv::Rect(col - 1, row - 1, 3, 3)).clone();
        std::cout << "sub_mat: " << std::endl
                  << debug_mat << std::endl;

        debug_mat = debug_mat.reshape(0, 1);
        cv::sort(debug_mat, debug_mat, cv::SORT_ASCENDING);
        std::cout << "sub_mat_sort: " << debug_mat << std::endl;
        //*/
    }

    void medianFilterImp(const cv::Mat &image, cv::Mat &output)
    {
        const int target_integral = 5;

        output = image.clone();
        for (int row = 1; row < image.rows - 1; row++)
        {
            uchar med_val;

            std::vector<int> patch_hist;
            int integral;

            for (int col = 1; col < image.cols - 1; col++)
            {
                if (col == 1)
                    initSorting(image, row, target_integral, patch_hist, med_val, integral);
                else
                    updateSorting(image, row, col, target_integral, patch_hist, med_val, integral);

                output.at<uchar>(row, col) = med_val;
            }
        }
    }

    void medianFilterImpSortEveryPatch(const cv::Mat &image, cv::Mat &output)
    {
        const int target_integral = 5;

        output = image.clone();
        for (int row = 1; row < image.rows - 1; row++)
        {
            for (int col = 1; col < image.cols - 1; col++)
            {
                uchar med_val;
                sortPatch(image, row, col, target_integral, med_val);

                output.at<uchar>(row, col) = med_val;
            }
        }
    }

    int medianFilter(std::string image_path)
    {

        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "conv_rank: error when reading image!" << std::endl;
            return -1;
        }

        // count time start median filter===========================
        cv::Mat output;
        double start = static_cast<double>(cv::getTickCount()); //

        medianFilterImp(image, output);

        // count time end
        double end = static_cast<double>(cv::getTickCount());
        double duration = ((double)(end - start) / cv::getTickFrequency());
        std::cout << "Duration median_filter_ours: " << duration * 1000 << " ms." << std::endl;

        cv::imshow("median_ours", output);

        // count time start median filter sorting every patch=======================
        cv::Mat output_dummy;
        double start1 = static_cast<double>(cv::getTickCount()); //

        medianFilterImpSortEveryPatch(image, output_dummy);

        // count time end
        double end1 = static_cast<double>(cv::getTickCount());
        double duration1 = ((double)(end1 - start1) / cv::getTickFrequency());
        std::cout << "Duration median_filter_ours_dummy: " << duration1 * 1000 << " ms." << std::endl;

        cv::imshow("median_ours_dummy", output_dummy);

        // count time start median filter OPENCV====================
        cv::Mat output_cv;
        double start2 = static_cast<double>(cv::getTickCount()); //

        cv::medianBlur(image, output_cv, 3);

        // count time end
        double end2 = static_cast<double>(cv::getTickCount());
        double duration2 = ((double)(end2 - start2) / cv::getTickFrequency());
        std::cout << "Duration median_filter_OpenCV: " << duration2 * 1000 << " ms." << std::endl;
        cv::imshow("median_cv", output_cv);

        // comparison of 3 results
        cv::Rect roi(1, 1, 7, 7);
        std::cout << "image: Left-Top: " << std::endl
                  << image(cv::Rect(roi.tl() - cv::Point(1, 1), roi.size() + cv::Size(2, 2))) << std::endl;

        std::cout << "median_ours (Left-Top): " << std::endl
                  << output(roi) << std::endl;

        std::cout << "median_ours_dummy (Left-Top): " << std::endl
                  << output_dummy(roi) << std::endl;

        std::cout << "median_cv (Left-Top): " << std::endl
                  << output_cv(roi) << std::endl;

        std::cout << "cv - ours (Left-Top): " << std::endl
                  << output_cv(roi) - output(roi) << std::endl;
        std::cout << "sum_diff: " << sum(output_cv(roi) - output(roi))[0] << std::endl;

        std::cout << "cv - ours_dummy (Left-Top): " << std::endl
                  << output_cv(roi) - output_dummy(roi) << std::endl;
        std::cout << "sum_diff_dummy: " << sum(output_cv(roi) - output_dummy(roi))[0] << std::endl;

        cv::waitKey(0);
    }

    void updateMedianFilter_onclass(const cv::Mat &image, int row, int col,int target,
                                    uchar &med_val, std::vector<int> &patch_hsit, int &integral)
    {
        cv::Point center(col, row);

        //left column=====
        cv::Rect roi( cv::Point(center.x -2, center.y-1), cv::Size(1, 3));
        std::vector<uchar> col_removed = image(roi).clone().reshape(0, 1);
        
        for(int i=0; i<3; i++)//update hist and integral
        {
            uchar gray = col_removed[i];
            patch_hsit[gray] --;
            if(gray <= med_val)
                integral --;
        }

        // right column=====
        // std::cout << "image 3 * 8:" << std::endl
        //           << image(cv::Rect(0, 0, 8, 3)) << std::endl << std::endl << std::endl;
        roi = cv::Rect(cv::Point(center.x + 1, center.y - 1), cv::Size(1, 3));
        std::vector<uchar> col_added = image(roi).clone().reshape(0, 1);

        for(int i=0; i<3; i++)//update hist and integral
        {
            uchar gray = col_added[i];
            patch_hsit[gray] ++;
            if(gray <= med_val)
                integral ++;
        }

        //update med_val
        if(integral < target)
        {
            do
            {
                med_val++;
                integral += patch_hsit[med_val];
            } while (integral < target);
        }
        else if(integral > target)
        {
            do
            {
                int temp_count = patch_hsit[med_val];
                if (integral - temp_count >= target)
                {
                    med_val--;
                    integral -= temp_count;
                }
                else
                    break;
            } while (integral > target);
        }
    }

    void initMedianFilter_onclass(const cv::Mat &image, int row, int col,int target,
                                  uchar &med_val, std::vector<int> &patch_hsit, int &integral)
    {
        //histogram
        patch_hsit = std::vector<int>(256, 0);
        
        for (int r = -1; r <= 1; r++)
        {
            for (int c = -1; c <= 1; c++)
            {
                uchar gray = image.at<uchar>( row + r,  col + c);
                // uchar gray = image.at<uchar>( cv::Point(col + c, row + r) );
                
                patch_hsit[gray] ++;

            }
        }

        // median value
        cv::Rect roi(cv::Point(col - 1, row - 1), cv::Size(3, 3));
        cv::Mat patch_array = image(roi).clone();

        std::cout << "first patch: " << std::endl
                  << patch_array << std::endl;

        int n_channels = 0;
        int n_new_rows = 1;
        std::vector<uchar> array = patch_array.reshape(n_channels,n_new_rows);

        std::sort(array.begin(), array.end());

        med_val = array[target-1];

        //integral
        integral = target;
        for (int i = target; i < 9; i++)
        {
            if (array[i] <= med_val)
                integral++;
        }
    }

    int medianFilter_onclass(std::string image_path)
    {
        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "conv_rank_onclass: error when reading image!" << std::endl;
            return -1;
        } 
        const int target_integral = 5;
 
        cv::Mat output = image.clone();

        double start = static_cast<double>(cv::getTickCount()); //

        for (int row = 1; row < image.rows - 1; row++)
        {
            uchar med_val;
            std::vector<int> patch_hsit;
            int integral;
            for (int col = 1; col < image.cols - 1; col++)
            {
                if (col == 1)
                    initMedianFilter_onclass(image, row, col,target_integral,
                                             med_val, patch_hsit, integral);
                else
                    updateMedianFilter_onclass(image, row, col, target_integral,
                                               med_val, patch_hsit, integral);

                output.at<uchar>(row, col) = med_val;
            }
        }

        // count time end
        double end = static_cast<double>(cv::getTickCount());
        double duration = ((double)(end - start) / cv::getTickFrequency());
        std::cout << "Duration ours: " << duration * 1000 << " ms." << std::endl;

        cv::Mat output_cv;
        start = static_cast<double>(cv::getTickCount()); //
        cv::medianBlur(image, output_cv,3);
         // count time end
        end = static_cast<double>(cv::getTickCount());
        duration = ((double)(end - start) / cv::getTickFrequency());
        std::cout << "Duration opencv: " << duration * 1000 << " ms." << std::endl;

        cv::Rect roi(1, 1, 10, 10);

        std::cout << "ours:" << std::endl
                  << output(roi) << std::endl;
        std::cout << "opencv:" << std::endl
                  << output_cv(roi) << std::endl;

        // roi = cv::Rect(1, 1, output.cols - 2, output.rows - 2);
        std::cout << "diff: " << cv::sum(output(roi) - output_cv(roi))[0] << std::endl;

        cv::imshow("m_ours", output);
        cv::imshow("m_cv", output_cv);
        cv::waitKey(0);

        
       
        return 0;
    }
}
