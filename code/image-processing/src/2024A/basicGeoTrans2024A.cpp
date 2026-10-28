#include <cstdio>
#include <cmath>

#include "opencv.hpp"

#include "basicGeoTrans2024A.h"

namespace DIPAutumn2024
{
    bool pointInRect(int x, int y, cv::Rect rc)
    {
        if (x < rc.tl().x)
            return false;
        if (y < rc.tl().y)
            return false;
        if (x >= rc.br().x)
            return false;
        if (y >= rc.br().y)
            return false;

        return true;
    }

    void invertAffineTransform(const cv::Mat &input, cv::Mat& input_inv)
    {
        //create homogeneous matrix
        cv::Mat input_homo = cv::Mat::zeros(3,3, CV_64F);
        input_homo.at<double>(0,0) = input.at<double>(0,0);
        input_homo.at<double>(0,1) = input.at<double>(0,1);
        input_homo.at<double>(0,2) = input.at<double>(0,2);
        
        input_homo.at<double>(1,0) = input.at<double>(1,0);
        input_homo.at<double>(1,1) = input.at<double>(1,1);
        input_homo.at<double>(1,2) = input.at<double>(1,2);
        
        input_homo.at<double>(2,0) = 0;
        input_homo.at<double>(2,1) = 0;
        input_homo.at<double>(2,2) = 1;

        cv::Mat input_inv_homo;
        cv::invert( input_homo, input_inv_homo);

        input_inv = cv::Mat::zeros(2,3, CV_64F);
        input_inv.at<double>(0,0) = input_inv_homo.at<double>(0,0);
        input_inv.at<double>(0,1) = input_inv_homo.at<double>(0,1);
        input_inv.at<double>(0,2) = input_inv_homo.at<double>(0,2);
        
        input_inv.at<double>(1,0) = input_inv_homo.at<double>(1,0);
        input_inv.at<double>(1,1) = input_inv_homo.at<double>(1,1);
        input_inv.at<double>(1,2) = input_inv_homo.at<double>(1,2);

        // return input_inv;
    }

    cv::Mat invertAffineTransform1(const cv::Mat &input)
    {
        cv::Mat input_homogeneous = cv::Mat::zeros(3, 3, CV_64F);
        input_homogeneous.at<double>(0, 0) = input.at<double>(0, 0);
        input_homogeneous.at<double>(0, 1) = input.at<double>(0, 1);
        input_homogeneous.at<double>(0, 2) = input.at<double>(0, 2);
        input_homogeneous.at<double>(1, 0) = input.at<double>(1, 0);
        input_homogeneous.at<double>(1, 1) = input.at<double>(1, 1);
        input_homogeneous.at<double>(1, 2) = input.at<double>(1, 2);
        input_homogeneous.at<double>(2, 0) = 0;
        input_homogeneous.at<double>(2, 1) = 0;
        input_homogeneous.at<double>(2, 2) = 1;

        cv::Mat output_homogeneous;
        cv::invert(input_homogeneous, output_homogeneous);

        cv::Mat output = cv::Mat::zeros(2, 3, CV_64F);
        output.at<double>(0, 0) = output_homogeneous.at<double>(0, 0);
        output.at<double>(0, 1) = output_homogeneous.at<double>(0, 1);
        output.at<double>(0, 2) = output_homogeneous.at<double>(0, 2);
        output.at<double>(1, 0) = output_homogeneous.at<double>(1, 0);
        output.at<double>(1, 1) = output_homogeneous.at<double>(1, 1);
        output.at<double>(1, 2) = output_homogeneous.at<double>(1, 2);
        return output;
    }

    cv::Point2f pointTransfrom(const cv::Point &input, const cv::Mat &matrix)
    {
        cv::Mat input_homogeneous = cv::Mat::ones(3, 1, CV_64F);
        input_homogeneous.at<double>(0, 0) = input.x;
        input_homogeneous.at<double>(1, 0) = input.y;
        input_homogeneous.at<double>(2, 0) = 1;

        // matrix -> 2 x 3

        // 2 x 1
        cv::Mat output_homogeneous = matrix * input_homogeneous;

        cv::Point2f output;
        output.x = output_homogeneous.at<double>(0, 0);
        output.y = output_homogeneous.at<double>(1, 0);

        return output;
    }

    cv::Vec3b getPixelValueNearestNeighbor(const cv::Mat &image, const cv::Point2f &pt_f)
    {
        cv::Point pt((int)(pt_f.x + 0.5), (int)(pt_f.y + 0.5));

        if (!pointInRect(pt.x, pt.y, cv::Rect(cv::Point(0, 0), image.size())))
            return cv::Vec3b(0, 0, 0);
        else
        {
            if (image.channels() == 1)
                return cv::Vec3b(image.at<uchar>(pt), image.at<uchar>(pt), image.at<uchar>(pt));
            else // if(image.channels() == 3 )
                return image.at<cv::Vec3b>(pt);
        }
    }

    cv::Vec3b getPixelValueBiLinear(const cv::Mat &image, const cv::Point2f &pt_f)
    {
        cv::Point pt((int)(pt_f.x + 0.5), (int)(pt_f.y + 0.5));

        if (!pointInRect(pt.x, pt.y, cv::Rect(cv::Point(0, 0), image.size())))
            return cv::Vec3b(0, 0, 0);
        else
        {
            int left = int(pt_f.x);
            int right = left + 1;
            int top = int(pt_f.y);
            int bot = top + 1;

            if (image.channels() == 1)
            {
                float pixel_tl = image.at<uchar>(cv::Point(left, top));
                float pixel_tr = image.at<uchar>(cv::Point(right, top));
                float pixel_bl = image.at<uchar>(cv::Point(left, bot));
                float pixel_br = image.at<uchar>(cv::Point(right, bot));

                float top_sum = (right - pt_f.x) * pixel_tl + (pt_f.x - left) * pixel_tr;
                float bot_sum = (right - pt_f.x) * pixel_bl + (pt_f.x - left) * pixel_br;
                uchar pixel_value = (bot - pt_f.y) * top_sum + (pt_f.y - top) * bot_sum;

                return cv::Vec3b(pixel_value, pixel_value, pixel_value);
            }
            else // if(image.channels() == 3 )
            {
                cv::Vec3b pixel_tl = image.at<cv::Vec3b>(cv::Point(left, top));
                cv::Vec3b pixel_tr = image.at<cv::Vec3b>(cv::Point(right, top));
                cv::Vec3b pixel_bl = image.at<cv::Vec3b>(cv::Point(left, bot));
                cv::Vec3b pixel_br = image.at<cv::Vec3b>(cv::Point(right, bot));

                cv::Vec3b pixel_value;
                for (int i = 0; i < 3; i++)
                {
                    float top_sum = (right - pt_f.x) * pixel_tl[i] + (pt_f.x - left) * pixel_tr[i];
                    float bot_sum = (right - pt_f.x) * pixel_bl[i] + (pt_f.x - left) * pixel_br[i];
                    pixel_value[i] = (bot - pt_f.y) * top_sum + (pt_f.y - top) * bot_sum;
                }

                return pixel_value;
            }
        }
    }

    void warpAffine_onclass(const cv::Mat &input, cv::Mat &output,
                    const cv::Mat &M_affine, cv::Size dst_size)
    {
        output = cv::Mat::zeros(dst_size, input.type());

        cv::Mat M_inverse_cv;
        std::cout << "Affine: " << std::endl
                  << M_affine << std::endl;

        cv::invertAffineTransform(M_affine, M_inverse_cv);
        std::cout << "Affine_inv_opencv: " << std::endl
                  << M_inverse_cv << std::endl;

        cv::Mat M_inverse;
        DIPAutumn2024::invertAffineTransform(M_affine, M_inverse);
        std::cout << "Affine_inv_ours: " << std::endl
                  << M_inverse << std::endl;
        
        int channels = input.channels();

        for (int row = 0; row < output.rows; row++)
        {
            for (int col = 0; col < output.cols; col++)
            {
                cv::Point pos(col,row);

                cv::Mat pos_homo = cv::Mat(3, 1, CV_64F);
                pos_homo.at<double>(0, 0) = pos.x;
                pos_homo.at<double>(1, 0) = pos.y;
                pos_homo.at<double>(2, 0) = 1;

                // std::cout << "pos_homo:" << std::endl
                //           << pos_homo << std::endl;

                cv::Mat pos_input_mat = M_inverse * pos_homo;
                // std::cout << "pos_input_mat:" << std::endl
                //           << pos_input_mat << std::endl;

                cv::Point2f pos_input_f;
                pos_input_f.x = pos_input_mat.at<double>(0, 0);
                pos_input_f.y = pos_input_mat.at<double>(1,0);

                cv::Point pos_nearest_neighbor;
                pos_nearest_neighbor.x = int(pos_input_f.x + 0.5);
                pos_nearest_neighbor.y = int(pos_input_f.y + 0.5);

                if(pos_nearest_neighbor.x < 0 )
                    continue;
                if(pos_nearest_neighbor.y < 0 )
                    continue;
                if(pos_nearest_neighbor.x >= input.cols )
                    continue;
                if(pos_nearest_neighbor.y >= input.rows )
                    continue;

                if(channels == 1)
                    output.at<uchar>(row,col) = input.at<uchar>(pos_nearest_neighbor);
                else// if(channels ==3)
                {
                    output.at<cv::Vec3b>(row, col) = input.at<cv::Vec3b>(pos_nearest_neighbor);
                }
            }
        }
    }

    void warpAffine(const cv::Mat &src, cv::Mat &dst, const cv::Mat &M_affine, cv::Size dst_size)
    {
        dst = cv::Mat::zeros(dst_size, src.type());

        cv::Mat M_inverse;
        std::cout << "M_affine: " << std::endl
                  << M_affine << std::endl;

        cv::invertAffineTransform(M_affine, M_inverse);
        std::cout << "m_inverse_opencv: " << std::endl
                  << M_inverse << std::endl;

        // cv::Mat M_inverse1 = invertAffineTransform(M_affine);
        // std::cout << "m_inverse_ours: " << std::endl
        //           << M_inverse1 << std::endl;

        for (int row = 0; row < dst.rows; row++)
        {
            for (int col = 0; col < dst.cols; col++)
            {
                cv::Point2f pt_src_f = pointTransfrom(cv::Point(col, row), M_inverse);
                cv::Vec3b color = getPixelValueBiLinear(src, pt_src_f);
                // cv::Vec3b color = getPixelValueNearestNeighbor(src, pt_src_f);

                if (src.channels() == 1)
                    dst.at<uchar>(row, col) = color[0];
                else // if(src.channels() ==3)
                    dst.at<cv::Vec3b>(row, col) = color;
            }
        }
    }

    int geoTransViaAffineMatrix(std::string image_path, std::string matrix_path)
    {
        double a[3], b[3];

        FILE *file = fopen(matrix_path.c_str(), "r+");
        fscanf(file, "%lf,%lf,%lf", &a[0], &a[1], &a[2]);
        fscanf(file, "%lf,%lf,%lf", &b[0], &b[1], &b[2]);
        fclose(file);

        cv::Mat affine_matrix = cv::Mat::zeros(2, 3, CV_64F);
        affine_matrix.at<double>(0, 0) = a[0];
        affine_matrix.at<double>(0, 1) = a[1];
        affine_matrix.at<double>(0, 2) = a[2];
        affine_matrix.at<double>(1, 0) = b[0];
        affine_matrix.at<double>(1, 1) = b[1];
        affine_matrix.at<double>(1, 2) = b[2];

        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "error when reading image!" << std::endl;
            return -1;
        }
        cv::imshow("image", image);
        cv::Mat result;
        warpAffine(image, result, affine_matrix, image.size() *2);
        cv::imshow("affine_result", result);
        cv::waitKey(0);

        return 0;
    }

    int geoTransViaAngleScaleTrans(std::string image_path, std::string params_path)
    {
        double deg, sx, sy, tx, ty;
        FILE *file = fopen(params_path.c_str(), "r+");
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

        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
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

    int loadPointsAndCalcAffine(std::string filepath, cv::Mat &model, cv::Size &dst_size)
    {
        std::vector<cv::Point2f> pts_src;
        std::vector<cv::Point2f> pts_dst;

        int x, y;

        std::FILE *file = std::fopen(filepath.c_str(), "r+");
        if (file == 0)
            return -1;

        std::fscanf(file, "%d %d", &x, &y);
        pts_src.push_back(cv::Point2f(x, y));
        std::fscanf(file, "%d %d", &x, &y);
        pts_src.push_back(cv::Point2f(x, y));
        std::fscanf(file, "%d %d", &x, &y);
        pts_src.push_back(cv::Point2f(x, y));
        std::fscanf(file, "%d %d", &x, &y);
        pts_src.push_back(cv::Point2f(x, y));

        std::fscanf(file, "%d %d", &x, &y);
        pts_dst.push_back(cv::Point2f(x, y));
        std::fscanf(file, "%d %d", &x, &y);
        pts_dst.push_back(cv::Point2f(x, y));
        std::fscanf(file, "%d %d", &x, &y);
        pts_dst.push_back(cv::Point2f(x, y));
        std::fscanf(file, "%d %d", &x, &y);
        pts_dst.push_back(cv::Point2f(x, y));

        std::fclose(file);

        model = cv::getAffineTransform(pts_src.data(), pts_dst.data());
        
        cv::getPerspectiveTransform(pts_src.data(), pts_dst.data());

        dst_size.width = pts_dst[1].x - pts_dst[0].x;
        dst_size.height = pts_dst[2].y - pts_dst[1].y;

        return 0;
    }

    int geoTransViaPoints(std::string image_path, std::string points_path)
    {
        // load and calculate affine transfrom
        cv::Size dst_size;
        cv::Mat affine_model;
        loadPointsAndCalcAffine(points_path, affine_model, dst_size);
        std::cout << "Affine Transform:" << std::endl;
        std::cout << affine_model << std::endl;

        cv::Mat image = cv::imread(image_path, cv::IMREAD_COLOR);
        // cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "Read image error!" << std::endl;
            return -1;
        }

        cv::imshow("image", image);

        cv::Mat result;
        warpAffine(image, result, affine_model, dst_size);

        cv::imshow("affine_result", result);
        cv::waitKey(0);

        return 0;
    }

}