

#include <string>
#include <iostream>

#include <cmath>

#include "opencv.hpp"

#include "Drawing.h"

#include "basicGrayTrans2024A.h"

namespace DIPAutumn2024
{
    float mappingGrayLog(float input, float start)
    {
        float output = log(input / 255.0 + start) * 255;

        if (output > 255)
            output = 255;
        if (output < 0)
            output = 0;

        return output;
    }

    float mappingGrayExp(float input, float start)
    {
        float output = (exp(input / 255.0) - start) * 255;

        if (output > 255)
            output = 255;
        if (output < 0)
            output = 0;

        return output;
    }

    float mappingGrayEqualHist(float input, cv::Size image_size, const cv::Mat &acc_hist)
    {
        float ratio = acc_hist.at<float>(input, 1) / (image_size.width * image_size.height);

        float output = ratio * 255;

        return output;
    }

    cv::Mat calcAccHist(const cv::Mat &image)
    {
        const int gray_rank = 256;
        cv::Mat hist = cv::Mat::zeros(gray_rank, 1, CV_32FC1);

        for (int row = 0; row < image.rows; row++)
        {
            for (int col = 0; col < image.cols; col++)
            {
                int gray = image.at<uchar>(row, col);
                hist.at<float>(gray, 1) += 1;
            }
        }
        std::cout << "hist: " << hist.t() << std::endl;

        cv::Mat acc_hist = cv::Mat::zeros(gray_rank, 1, CV_32FC1);
        for (int gray = 0; gray < gray_rank; gray++)
        {
            if (gray == 0)
                acc_hist.at<float>(gray, 1) = hist.at<float>(gray, 1);
            else
                acc_hist.at<float>(gray, 1) =
                    acc_hist.at<float>(gray - 1, 1) + hist.at<float>(gray, 1);
        }
        std::cout << "acc hist: " << acc_hist.t() << std::endl;

        return acc_hist;
    }

    int grayTrans(std::string dir, std::string name, float start)
    {
        cv::Mat image = cv::imread(dir + name, cv::IMREAD_GRAYSCALE);

        if (image.empty())
        {
            std::cout << "error: image is empty!" << std::endl;
            return -1;
        }

        cv::imshow("image", image);
        cvrsg::common::Drawing::DrawAndShowHist(image, "hist");

        // compute accumulated histogram
        cv::Mat acc_hist = calcAccHist(image);

        cv::Mat trans_image = cv::Mat::zeros(image.size(), CV_8UC1);

        for (int row = 0; row < image.rows; row++)
        {
            for (int col = 0; col < image.cols; col++)
            {
                float input = image.at<uchar>(row, col);
                float output = mappingGrayEqualHist(input, image.size(), acc_hist);
                trans_image.at<uchar>(row, col) = output;
            }
        }

        cv::imshow("EqualHist", trans_image);
        cvrsg::common::Drawing::DrawAndShowHist(trans_image, "hist_equal");

        cv::Mat trans_image_cv;
        cv::equalizeHist(image, trans_image_cv);
        cv::imshow("EqualHist_CV", trans_image_cv);
        cvrsg::common::Drawing::DrawAndShowHist(trans_image_cv, "hist_equal_cv");

        return 0;
    }

    int calcHist(std::string path)
    {
        cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);

        if (image.empty())
        {
            std::cout << "Error: Image is empty, check the path!" << std::endl;
            return -1;
        }

        cv::imshow("IMAGE", image);
        cv::waitKey(0);

        const int gray_rank = 256;

        cv::Mat hist_mat = cv::Mat::zeros(gray_rank, 1, CV_32F);

        float hist[gray_rank];
        memset(hist, 0, sizeof(float) * gray_rank);

        for (int row = 0; row < image.rows; row++)
        {
            for (int col = 0; col < image.cols; col++)
            {
                int gray = image.at<uchar>(row, col);
                // int gray1 = image.at<uchar>(cv::Point2d(col, row));

                hist[gray] += 1;
                hist_mat.at<float>(gray, 0) += 1;
            }
        }

        std::cout << "Histogram: " << hist_mat.t() << std::endl;

        cvrsg::common::Drawing::DrawAndShowHist(image);

        int target_gray = 28;

        cv::Mat result = image.clone();
        for (int row = 0; row < image.rows; row++)
        {
            for (int col = 0; col < image.cols; col++)
            {
                int gray = result.at<uchar>(row, col);
                if (abs(gray - target_gray) > 5)
                    result.at<uchar>(row, col) = 255;
            }
        }

        cv::imshow("gray=28", result);
        cv::waitKey(0);

        return 0;
    }

    cv::Mat calcAccHistOld(cv::Mat image)
    {
        const int gray_rank = 256;

        cv::Mat hist = cv::Mat::zeros(gray_rank, 1, CV_32F);
        for (int row = 0; row < image.rows; row++)
        {
            for (int col = 0; col < image.cols; col++)
            {
                int gray = image.at<uchar>(row, col);
                hist.at<float>(gray, 0) += 1;
            }
        }

        cv::Mat acc_hist = cv::Mat::zeros(gray_rank, 1, CV_32F);
        for (int gray = 0; gray < gray_rank; gray++)
        {
            if (gray == 0)
                acc_hist.at<float>(gray, 0) = hist.at<float>(gray, 0);
            else
                acc_hist.at<float>(gray, 0) =
                    acc_hist.at<float>(gray - 1, 0) + hist.at<float>(gray, 0);
        }

        for (int gray = 0; gray < gray_rank; gray++)
            acc_hist.at<float>(gray, 0) /= (image.cols * image.rows);

        return acc_hist;
    }

    float mappingLog(float input, float start = 1.0)
    {
        float output = log(input / 255.0 + start) * 255;
        if (output > 255)
            output = 255;
        if (output < 0)
            output = 0;
        return output;
    }

    float mappingExp(float input, float start = 1.0)
    {
        float output = (exp(input / 255.0) - start) * 255;
        if (output > 255)
            output = 255;
        if (output < 0)
            output = 0;
        return output;
    }

    float mappingEqualizeHist(float input, cv::Mat acc_hist)
    {
        float output = acc_hist.at<float>(input) * 255;
        return output;
    }

    int grayTransfrom(std::string dir, std::string file_name, int trans_type, float param)
    {
        cv::Mat image = cv::imread(dir + file_name, cv::IMREAD_GRAYSCALE);

        if (image.empty())
        {
            std::cout << "Error: Image is empty, check the path!" << std::endl;
            return -1;
        }

        cv::resize(image, image, cv::Size(image.cols * 1.5, image.rows * 1.5));

        cv::imshow(file_name, image);
        cvrsg::common::Drawing::DrawAndShowHist(image, "Hist");
        // cv::waitKey(0);

        cv::Mat new_image = cv::Mat::zeros(image.size(), CV_8UC1);

        cv::Mat acc_hist = calcAccHist(image);

        std::string method_name = "";
        for (int row = 0; row < image.rows; row++)
        {
            for (int col = 0; col < image.cols; col++)
            {
                float gray = image.at<uchar>(row, col);

                switch (trans_type)
                {
                case LOG:
                    new_image.at<uchar>(row, col) = mappingLog(gray, param);
                    break;
                case EXP:
                    new_image.at<uchar>(row, col) = mappingExp(gray, param);
                    break;
                case EQUALIZEHIST:
                    new_image.at<uchar>(row, col) = mappingEqualizeHist(gray, acc_hist);
                    break;
                }
            }
        }

        switch (trans_type)
        {
        case LOG:
            cv::imshow("LOG", new_image);
            cvrsg::common::Drawing::DrawAndShowHist(new_image, "Hist_LOG");
            break;
        case EXP:
            cv::imshow("EXP", new_image);
            cvrsg::common::Drawing::DrawAndShowHist(new_image, "Hist_EXP");
            break;
        case EQUALIZEHIST:
            cv::imshow("EqualizeHist_Ours", new_image);
            cvrsg::common::Drawing::DrawAndShowHist(new_image, "Hist_Equal");

            cv::Mat new_image_opencv;
            cv::equalizeHist(image, new_image_opencv);
            cv::imshow("EqualizeHist_OpenCV", new_image_opencv);
            cvrsg::common::Drawing::DrawAndShowHist(new_image, "Hist_Equal_OpenCV");
            break;
        }

        cv::waitKey(0);

        return 0;
    }
}
