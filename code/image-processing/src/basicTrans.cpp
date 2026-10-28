#include "basicTrans.h"

#include "utilities.h"
#include "Drawing.h"
#include <string>

namespace cvrsg
{
    namespace kongzi
    {
        namespace transform
        {
            int ImageHistEqualExercise(std::string path)
            {
                //to fill: image histogram equalization
                
                // end of the hist equalization
                return 0;
            }

            int HistEqual(std::string path)
            {
                cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);

                if (image.empty())
                {
                    std::cout << "error: reading file failed!" << std::endl;
                    return -1;
                }

                cv::imshow("image", image);
                cvrsg::common::Drawing::DrawAndShowHist(image, "Histogram", 256, 256);
                // cv::waitKey(0);

                //
                const int gray_ranks = 256;

                // Step1: Comptue input histogram
                float hist[gray_ranks] = {0};
                memset(hist, 0, sizeof(float) * gray_ranks);

                for (int row = 0; row < image.rows; row++)
                {
                    for (int col = 0; col < image.cols; col++)
                    {
                        uchar gray = image.at<uchar>(row, col);
                        hist[gray] += 1;
                    }
                }

                // Step2: Compute cumulative histogram
                float hist_cum[gray_ranks] = {0};
                hist_cum[0] = hist[0];
                for (int i = 1; i < gray_ranks; i++)
                {
                    hist_cum[i] = hist_cum[i - 1] + hist[i];
                }

                // Step3: Compute output gray using cumulative histogram
                cv::Mat result = image.clone();
                float image_size = image.cols * image.rows;

                for (int row = 0; row < image.rows; row++)
                {
                    for (int col = 0; col < image.cols; col++)
                    {
                        uchar gray_input = image.at<uchar>(row, col);

                        uchar gray_output = hist_cum[gray_input] / image_size * 255;

                        result.at<uchar>(row, col) = gray_output;
                    }
                }

                cv::imshow("Equal Image", result);
                cvrsg::common::Drawing::DrawAndShowHist(result, "Equal Hist", 256, 256);
                // cv::waitKey(0);

                return 0;
            }

            cv::Point2f TransferPointUsingAffine(const cv::Point2f &input_point, const cv::Mat &affine_model)
            {
                cv::Mat input_point_m = cv::Mat::ones(3, 1, CV_64F);
                input_point_m.at<double>(0, 0) = input_point.x;
                input_point_m.at<double>(1, 0) = input_point.y;

                cv::Mat output_point_m = affine_model * input_point_m;

                cv::Point2f output_point;
                output_point.x = output_point_m.at<double>(0, 0);
                output_point.y = output_point_m.at<double>(1, 0);

                return output_point;
            }

            void WarpImage(const cv::Mat &input, cv::Mat &output, const cv::Mat &model_inverse)
            {

                for (int row = 0; row < output.rows; row++)
                {
                    for (int col = 0; col < output.cols; col++)
                    {
                        cv::Point2f output_pt(col, row);

                        cv::Mat output_pt_homo = cv::Mat::ones(3, 1, CV_64F);
                        
                        output_pt_homo.at<double>(0, 0) = output_pt.x;
                        output_pt_homo.at<double>(1, 0) = output_pt.y;
                        output_pt_homo.at<double>(2, 0) = 1;

                        cv::Mat input_pt_homo = model_inverse * output_pt_homo;

                        cv::Point2f input_pt;
                        input_pt.x = input_pt_homo.at<double>(0, 0);
                        input_pt.y = input_pt_homo.at<double>(1, 0);

                        // cv::Point2f input_ptf = TransferPointUsingAffine(output_ptf, model_inverse);
                        
                        //nearest neighbor interpolation
                        int input_pt_x = (int)(input_pt.x + 0.5);
                        int input_pt_y = (int)(input_pt.y + 0.5);

                        if (input_pt_x < 0)
                            continue;
                        if (input_pt_y < 0)
                            continue;
                        if (input_pt_x > input.cols - 1)
                            continue;
                        if (input_pt_y > input.rows - 1)
                            continue;

                        output.at<uchar>(row, col) = input.at<uchar>(input_pt_y, input_pt_x);

                    }
                }

                // cv::imshow("output", output);
                // cv::waitKey(0);


            }

            int CalcTransfromAndWarpExercise(std::string file_path)
            {
                cv::Mat image = cv::imread(file_path, cv::IMREAD_GRAYSCALE);
                if (image.empty())
                {
                    std::cout << "read image failed!" << std::endl;
                    return -1;
                }

                //根据同名点定义变换
                const int pts_number = 3; //affine transform  for 3 points
                cv::Point2f pts_src[pts_number] = { cv::Point2f(10, 10), //source point is (10, 10)
                    cv::Point2f(image.cols - 1 - 10, 10), //source point is (width-10, 10)
                    cv::Point2f(10, image.rows - 1 - 10)}; //source point is (10, height-10)
                
                cv::Point2f pts_dst[pts_number] = { cv::Point2f(20, 100), //destination point is (20, 100)
                    cv::Point2f(image.cols / 2, 20), //destination point is (width/2, 20)
                    cv::Point2f(50, image.rows / 3 * 2)}; //destination point is (50, height/3*2)

                cv::Mat image_dst;
                
                //使用仿射变换进行图像变换，使用自己编写warpAffine函数
                {
                    //计算仿射变换模型
                    cv::Mat model = cv::getAffineTransform(pts_src, pts_dst);

                    std::cout << "Affine Model: " << std::endl;
                    std::cout << model << std::endl;
                    std::cout << std::endl;

                    //计算仿射变换模型的逆矩阵
                    cv::Mat model_inverse;
                    cv::invertAffineTransform(model, model_inverse);
                    std::cout << "Inverse Affine Model: " << std::endl;
                    std::cout << model_inverse << std::endl;
                    std::cout << std::endl;

                    //计算变换后的图像的边界
                    int left = pts_dst[0].x;
                    int top = pts_dst[0].y;
                    int right = left;
                    int bottom = top;
                    for (int i = 1; i < pts_number; i++)
                    {
                        if (pts_dst[i].x < left)
                            left = pts_dst[i].x;
                        if (pts_dst[i].x > right)
                            right = pts_dst[i].x;
                        if (pts_dst[i].y < top)
                            top = pts_dst[i].y;
                        if (pts_dst[i].y > bottom)
                            bottom = pts_dst[i].y;
                    }

                    //计算变换后的图像的大小
                    cv::Size dst_size((right - left)*2, (bottom - top)*2); //放大2倍,以防显示画布不够大
                    image_dst = cv::Mat::zeros(dst_size, CV_8UC1);

                    //进行仿射变换
                    WarpImage(image, image_dst, model_inverse);
                }
               
            
                //创建彩色的原始图像
                cv::Mat image_rgb = image.clone();
                cv::cvtColor(image, image_rgb, cv::COLOR_GRAY2BGR);
                
                //创建变换后的彩色图像
                cv::Mat image_dst_rgb = image_dst.clone();
                cv::cvtColor(image_dst, image_dst_rgb, cv::COLOR_GRAY2BGR);

                //绘制原始图像和变换后的图像的同名点
                for (int i = 0; i < pts_number; i++)
                {
                    cv::circle(image_rgb, pts_src[i], 5, cv::Scalar(0, 0, 255), 2);
                    cv::circle(image_dst_rgb, pts_dst[i], 5, cv::Scalar(0, 255, 0), 2);
                }

                //显示原始图像和变换后的图像
                cv::imshow("image_src", image_rgb); 
                cv::imshow("image_dst", image_dst_rgb);
                cv::waitKey(0);

                return 0;
            }


            void WarpAffineDIP(const cv::Mat& src, cv::Mat& dst, const cv::Mat& M_affine, cv::Size dst_size)
            {
                
                dst = cv::Mat(dst_size, CV_8UC1);

                cv::Mat M_inverse;
                cv::invertAffineTransform(M_affine, M_inverse);

                for (int row = 0; row < dst.rows; row++)
                {
                    for (int col = 0; col < dst.cols; col++)
                    {
                        int dst_x = col;
                        int dst_y = row;

                        //3 x 1
                        cv::Mat dst_pt_h = cv::Mat::ones(3, 1, CV_64F);
                        dst_pt_h.at<double>(0, 0) = dst_x;
                        dst_pt_h.at<double>(1, 0) = dst_y;
                        dst_pt_h.at<double>(2, 0) = 1;

                        //M_inverse -> 2 x 3

                        //3 x 1
                        cv::Mat src_pt_h = M_inverse * dst_pt_h;

                        double src_pt_xf = src_pt_h.at<double>(0, 0);
                        double src_pt_yf = src_pt_h.at<double>(1, 0);

                        int src_pt_x = (int)(src_pt_xf + 0.5);
                        int src_pt_y = (int)(src_pt_yf + 0.5);

                        if(src_pt_x < 0)
                            continue;
                        if(src_pt_y < 0)
                            continue;
                        if(src_pt_x > src.cols -1)
                            continue;
                        if(src_pt_y > src.rows -1)
                            continue;
                            

                        uchar gray = src.at<uchar>(src_pt_y, src_pt_x);
                        dst.at<uchar>(row, col) = gray;


                    }
                }
            }

            int CalcTransfromAndWarp(std::string file_path, int pts_number,
                                     int *pts_src_x, int *pts_src_y,
                                     int *pts_dst_x, int *pts_dst_y)
            {
                cv::Mat image = cv::imread(file_path, cv::IMREAD_GRAYSCALE);
                if(image.empty())
                {
                    std::cout<<"file read error!"<<std::endl;
                    return -1;
                }

                std::vector<cv::Point2f> pts_src, pts_dst;
                for(int i=0; i<pts_number; i++)
                {
                    pts_src.push_back(  cv::Point2f( pts_src_x[i] , pts_src_y[i]) );
                    pts_dst.push_back(  cv::Point2f( pts_dst_x[i] , pts_dst_y[i]) );
                }

                cv::Mat model_affine =
                    cv::getAffineTransform(pts_src.data(), pts_dst.data());

                std::cout << "Affine Transform:" << std::endl;
                std::cout << model_affine << std::endl;

                cv::Mat dst_image;
                // cv::warpAffine(image, dst_image, model_affine, image.size()*2);

                int left = pts_dst[0].x;
                int right =left;
                int top = pts_dst[0].y;
                int bottom= top;

                for(int i=0; i<pts_number; i++)
                {
                    if(left > pts_dst[i].x)
                        left = pts_dst[i].x;
                    if(right < pts_dst[i].x)
                        right = pts_dst[i].x;
                    if(top > pts_dst[i].y)
                        top = pts_dst[i].y;
                    if(bottom < pts_dst[i].y)
                        bottom = pts_dst[i].y;
                }
                
                cv::Size dst_size(right, bottom);
                WarpAffineDIP(image, dst_image, model_affine, dst_size*2);

                cv::imshow("image", image);
                cv::imshow("affine", dst_image);
                cv::waitKey(0);

                return 0;
            }

            int CalcAffineTransform(std::string file_path)
            {
                cv::Mat image = cv::imread(file_path, cv::IMREAD_GRAYSCALE);
                if (image.empty())
                {
                    std::cout << "read image failed!" << std::endl;
                    return -1;
                }

                cv::imshow("image", image);
                cv::waitKey(0);

                const int pts_number_affine = 3;
                cv::Point2f pts_src[pts_number_affine];
                pts_src[0] = cv::Point2f(10, 10);
                pts_src[1] = cv::Point2f(image.cols - 1 - 10, 10);
                pts_src[2] = cv::Point2f(10, image.rows - 1 - 10);

                cv::Point2f pts_dst[pts_number_affine];
                pts_dst[0] = cv::Point2f(20, 100);                // src point is (10,10)
                pts_dst[1] = cv::Point2f(image.cols / 2, 20);     // src point is (width-10, 10)
                pts_dst[2] = cv::Point2f(50, image.rows / 3 * 2); // src point is (10, height-10)

                // Demo 1: compute affine model and warp the image
                cv::Mat affine_model = cv::getAffineTransform(pts_src, pts_dst);

                std::cout << std::endl;
                std::cout << "===========================================================" << std::endl;
                std::cout << "Demo1: compute affine from 3 pairs of points:" << std::endl;
                std::cout << "Source Points: " << pts_src[0] << ", " << pts_src[1] << ", " << pts_src[2] << std::endl;
                std::cout << "Dest Points: " << pts_dst[0] << ", " << pts_dst[1] << ", " << pts_dst[2] << std::endl;

                std::cout << "Affine Model: " << std::endl;
                std::cout << affine_model << std::endl;
                std::cout << "===========================================================" << std::endl;
                std::cout << std::endl;

                cv::Mat image_dst = image.clone();
                warpAffine(image, image_dst, affine_model, cv::Size(image.cols, image.rows));

                cv::Mat image_rgb = image.clone();
                cv::cvtColor(image, image_rgb, cv::COLOR_GRAY2BGR);
                cv::Mat image_dst_rgb = image_dst.clone();
                cv::cvtColor(image_dst, image_dst_rgb, cv::COLOR_GRAY2BGR);

                for (int i = 0; i < pts_number_affine; i++)
                {
                    cv::circle(image_rgb, pts_src[i], 5, cv::Scalar(0, 255, 255), 2);
                    cv::circle(image_dst_rgb, pts_dst[i], 5, cv::Scalar(0, 255, 255), 2);
                }

                cv::imshow("image", image_rgb);
                cv::imshow("image_dst", image_dst_rgb);
                cv::waitKey(0);

                // Demo2: compute given point to dst image using the affine model above
                cv::Point computed_points[3];

                for (int i = 0; i < 3; i++)
                    computed_points[i] = TransferPointUsingAffine(pts_src[i], affine_model);

                cv::Point given_point(image.cols / 2, image.rows / 2);
                cv::Point warped_point = TransferPointUsingAffine(given_point, affine_model);

                std::cout << "===========================================================" << std::endl;
                std::cout << "Demo2: compute given points to dest image:" << std::endl;
                std::cout << "Source Points: " << pts_src[0] << ", " << pts_src[1] << ", " << pts_src[2] << std::endl;
                std::cout << "Dest Points: " << pts_dst[0] << ", " << pts_dst[1] << ", " << pts_dst[2] << std::endl;
                std::cout << "Computed Points: " << computed_points[0] << ", " << computed_points[1] << ", " << computed_points[2] << std::endl;

                std::cout << "Given Point: " << given_point << std::endl;
                std::cout << "Warped Point: " << warped_point << std::endl;

                std::cout << "===========================================================" << std::endl;
                std::cout << std::endl;

                cv::circle(image_rgb, given_point, 4, cv::Scalar(0, 0, 255), 3);
                cv::circle(image_dst_rgb, warped_point, 4, cv::Scalar(0, 0, 255), 3);
                cv::imshow("image", image_rgb);
                cv::imshow("image_dst", image_dst_rgb);
                cv::waitKey(0);

                // Demo 3: compute the inverse model
                cv::Mat affine_model_inverse;
                cv::invertAffineTransform(affine_model, affine_model_inverse);
                cv::Mat affine_model_inverse1;
                affine_model_inverse1 = cv::getAffineTransform(pts_dst, pts_src);

                std::cout << "===========================================================" << std::endl;
                std::cout << "Demo3: compute inverse model:" << std::endl;
                std::cout << "invertAffineTransform:" << std::endl;
                std::cout << affine_model_inverse << std::endl;
                std::cout << "getAffineTransform(pts_dst, pts_src):" << std::endl;
                std::cout << affine_model_inverse1 << std::endl;
                std::cout << "===========================================================" << std::endl;
                std::cout << std::endl;

                return 0;
            }

            int RotateScaleTranslationExercise(std::string path, double angle_deg,
                                                      double trans_x,
                                                      double trans_y,
                                                      double scale_x,
                                                      double scale_y)
            {

                cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
                if (image.empty())
                {
                    std::cout << "Read image error!" << std::endl;
                    return -1;
                }

                cv::Mat affine_matrix(2, 3, CV_64FC1); //2 x 3 affine matrix

                double angle_rad = angle_deg / 180 * CV_PI;

                // | a0 a1 a2 |
                // | b0 b1 b2 |
                //
                // | -1 0 image.cols - 1 |
                // | 0 -1 image.rows - 1 |
                //
                affine_matrix.at<double>(0, 0) = -1;             // a0 = -1
                affine_matrix.at<double>(0, 1) = 0;              // a1 = 0
                affine_matrix.at<double>(0, 2) = image.cols - 1; // a2 = image.cols - 1

                affine_matrix.at<double>(1, 0) = 0;              // b0 = 0
                affine_matrix.at<double>(1, 1) = -1;             // b1 = -1
                affine_matrix.at<double>(1, 2) = image.rows - 1; // b2 = image.rows - 1

                std::cout << "Affine Model:" << std::endl;
                std::cout << affine_matrix << std::endl;

                cv::Mat aff_image;
                cv::warpAffine(image, aff_image, affine_matrix, image.size() * 2);

                cv::imshow("Image", image);
                cv::imshow("AffineImage", aff_image);
                cv::waitKey(0);
                return 0;
            }

            int WarpImageUinsgAffineExercise(std::string file_path,
                                              double angle_deg,
                                              double trans_x,
                                              double trans_y,
                                              double scale_x,
                                              double scale_y)
            {
                cv::Mat image = cv::imread(file_path, cv::IMREAD_GRAYSCALE);
                if (image.empty())
                {
                    std::cout << "read image failed!" << std::endl;
                    return -1;
                }

                cv::imshow("image", image);

                //计算旋转角度
                double angle_rad = angle_deg / 180 * CV_PI;

                //计算仿射变换模型
                cv::Mat affine_model = cv::Mat::zeros(2, 3, CV_64F); //2 x 3 affine matrix
                
                // | scale_x * cos(angle_rad),          -sin(angle_rad),  trans_x |
                // |           sin(angle_rad), scale_y * cos(angle_rad),  trans_y |
                //
                affine_model.at<double>(0, 0) = scale_x * cos(angle_rad); //a0 = scale_x * cos(angle_rad)
                affine_model.at<double>(0, 1) = -sin(angle_rad); //a1 = -sin(angle_rad)
                affine_model.at<double>(0, 2) = trans_x; //a2 = trans_x

                affine_model.at<double>(1, 0) = sin(angle_rad); //b0 = sin(angle_rad)
                affine_model.at<double>(1, 1) = scale_y * cos(angle_rad); //b1 = scale_y * cos(angle_rad)
                affine_model.at<double>(1, 2) = trans_y; //b2 = trans_y

                //显示仿射变换模型
                std::cout << "R: " << angle_deg << " deg" << std::endl;
                std::cout << "T: x = " << trans_x << " pix, y = " << trans_y << " pix" << std::endl;
                std::cout << "S: x = " << scale_x << ", y = " << scale_y << std::endl;

                std::cout << "Affile Model:" << std::endl;
                std::cout << affine_model << std::endl;

                //进行仿射变换
                cv::Mat image_dst = image.clone();
                cv::warpAffine(image, image_dst, affine_model, cv::Size(image.cols * 2, image.rows * 2));
                cv::imshow("Warped image", image_dst);
                cv::waitKey(0);

                return 0;
            }
        }
    }
}