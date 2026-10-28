#ifndef _CVRSG_COMMON_DRAWING_H
#define _CVRSG_COMMON_DRAWING_H

#include <string>
#include <opencv.hpp>
#include <direct.h>

namespace cvrsg
{
    namespace common
    {
        // _declspec(dllexport)
        class Drawing
        {
        public:
            Drawing(){};
            ~Drawing(){};

            static void PutText(cv::Mat &image, cv::Point pos, std::string text, cv::Scalar color = cv::Scalar(0, 0, 255), float scale = 1.0, int thickness = 1, bool fill_bkg = false);

            static void VisualizeMatrix(const cv::Mat &src, std::string window_name, float alpha);

            static void DrawRotatedRect(const cv::Mat &image, cv::RotatedRect rrc, cv::Scalar color, int thickness);

            static void DrawHist(const cv::Mat &image, cv::Mat &hist_image, int histWidth = 400, int histHeight = 400);

            // hist -- CV_32FC1
            static void DrawHist(const cv::Mat &hist, cv::Mat &histImage, const std::vector<int> &maxima_indices = std::vector<int>(),
                                 cv::Scalar color = cv::Scalar(0, 255, 0), int margin = 30, int xTickStep = 10, int yTickStep = 0.5);

            static void DrawHist(const cv::Mat &src_hist, cv::Mat &histImage,
                                 const std::vector<std::pair<int, double>> &maximas, cv::Scalar color = cv::Scalar(0, 255, 0),
                                 int margin = 30, int xTickStep = 10, int yTickStep = 0.5);

            static void DrawHistBars(const cv::Mat &src_hist, cv::Mat &histImage,
                                     const std::vector<std::pair<int, double>> &maximas, int dst_width = -1,
                                     cv::Scalar color = cv::Scalar(0, 255, 0),
                                     int margin = 30, int xTickStep = 10, int yTickStep = 0.5);

            static void DrawAndShowHist(const cv::Mat &image, std::string windowName = "Histogram", int histWidth = 400, int histHeight = 400);

            static void DrawLOG(cv::Mat &image, std::string text, float scale = 0.8, int thickness = 2);
        };
    }
}

#endif //_CVRSG_COMMON_DRAWING_H