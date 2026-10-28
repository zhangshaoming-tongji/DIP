#ifndef _CVRSG_COMMON_UTILITIES_H
#define _CVRSG_COMMON_UTILITIES_H

#include <string>
#include <opencv.hpp>
#include <direct.h>
#include <vector>
#include <chrono>





namespace cvrsg
{
    namespace common
    {
        namespace utilities
        {
            int parseArgsByLines(std::string args_file_path, std::vector<std::string> &args);

            std::string getCurrentSystemTime();
            std::vector<std::string>  GetTimeStamp_String();
            std::time_t GetTimeStamp_Milliseconds(int& ms);
            

            std::time_t getTimeStamp(int &ms);
            std::tm *gettm(long long timestamp);
            std::string tmToString(std::tm* ptm);
            
           
            std::string to_string(double val, int fixed);
            std::string to_string(int val);

            std::string toLower(std::string src);
            std::string toUpper(std::string src);

            //注意这两个函数是否有内存访问越界，heap或stack崩溃
            std::wstring StringToWString(const std::string &str);
            std::string WStringToString(const std::wstring &wstr);

            //根据first的值降序排序
            bool cmp1(std::pair<int, int> a, std::pair<int, int> b);

            //根据second的值降序排序
            bool cmp2(std::pair<int, int> a, std::pair<int, int> b);

            bool IsSafeRect(const cv::Mat &image, cv::Rect rc);
            cv::Rect AdjustToSafeRect(const cv::Mat &image, cv::Rect rc);
            cv::Rect getSafeRectSizeFixed(cv::Rect &rc, const cv::Mat &image);
            cv::Rect getSafeRect(cv::Rect &rc, const cv::Mat &image);
            cv::Rect getSafeRect(cv::Rect &rc, const cv::Size &size);

            int findLocalMaximasLoc(const std::vector<double> &src, std::vector<int> &output_indices,
                                    const int minPeakDistance, const double minHeightDiff, const double minPeakHeight);

            int findLocalMaximasLoc(const cv::Mat &src, std::vector<int> &output_indices,
                            const int minPeakDistance, const double minHeightDiff, const double minPeakHeight);
           
            int findLocalMaximas(const cv::Mat &src, std::vector< std::pair<int, double> > &output_peaks,
                            const int minPeakDistance, const double minHeightDiff, const double minPeakHeight);


            
            //read string
            bool SplitString(const std::string& src, const std::string& flag, std::vector<std::string>& dst);
            void TrimString(std::string &str);
            double MinValue();
            double Epsilon();

        }
    }
}

#endif //_CVRSG_COMMON_UTILITIES_H