#include "utilities.h"

#include <windows.h>
#include <sys/timeb.h>
#include <io.h>
#include <direct.h>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>

#include <vector>
#include <algorithm>
#include <cctype>
#include <ratio>
#include <chrono>
#include <limits.h>

#include <utility>

#define MAX_PATH 260

namespace cvrsg
{
    namespace common
    {
        // using namespace::cvrsg::common;

        namespace utilities
        {

            // 需要开启c++11支持，g++ test.cpp -std=c++11 -o test， 这里默认是东八区北京时间格式

            // using namespace std;

            std::time_t GetTimeStamp_Milliseconds(int &millisecond)
            {
                std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
                auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
                auto tmp_s = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());

                millisecond = tmp.count() - tmp_s.count() * 1000;

                std::time_t timestamp = tmp.count();
                // std::time_t timestamp = std::chrono::system_clock::to_time_t(tp);
                return timestamp;
            }

            std::vector<std::string> GetTimeStamp_String()
            {
                using namespace std;

                int millisecond;
                std::time_t time_stamp = GetTimeStamp_Milliseconds(millisecond);
                std::tm *ptm = gettm(time_stamp);

                std::vector<string> time_stamp_str;

                time_stamp_str.push_back(std::to_string(ptm->tm_year + 1900));

                string mon_str = std::to_string(ptm->tm_mon + 1);
                mon_str = std::string(2 - mon_str.length(), '0') + mon_str;
                time_stamp_str.push_back(mon_str);

                string day_str = std::to_string(ptm->tm_mday);
                day_str = std::string(2 - day_str.length(), '0') + day_str;
                time_stamp_str.push_back(day_str);

                string hour_str = std::to_string(ptm->tm_hour);
                hour_str = std::string(2 - hour_str.length(), '0') + hour_str;
                time_stamp_str.push_back(hour_str);

                string min_str = std::to_string(ptm->tm_min);
                min_str = std::string(2 - min_str.length(), '0') + min_str;
                time_stamp_str.push_back(min_str);

                string sec_str = std::to_string(ptm->tm_sec);
                sec_str = std::string(2 - sec_str.length(), '0') + sec_str;
                time_stamp_str.push_back(sec_str);

                time_stamp_str.push_back(std::to_string(millisecond));

                return time_stamp_str;
            }

            std::tm *gettm(long long timestamp)
            {
                auto milli = timestamp + (long long)8 * 60 * 60 * 1000; //此处转化为东八区北京时间，如果是其它时区需要按需求修改
                auto mTime = std::chrono::milliseconds(milli);
                auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
                auto tt = std::chrono::system_clock::to_time_t(tp);
                std::tm *now = std::gmtime(&tt);
                // printf("%4d年%02d月%02d日 %02d:%02d:%02d\n",
                // now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
                return now;
            }

            std::string tmToString(tm *ptm)
            {
                // auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                // struct tm *ptm = localtime(&tt);
                char date[60] = {0};
                sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
                        (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
                        (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
                return std::string(date);
            }

            std::string getCurrentSystemTime()
            {
                int ms;
                std::time_t time_stamp = GetTimeStamp_Milliseconds(ms);
                std::tm *ptm = gettm(time_stamp);
                return tmToString(ptm);
            }

            void Test_duration()
            {

                using namespace std::chrono;

                steady_clock::time_point t1 = steady_clock::now();

                std::cout << "printing out 1000 stars...\n";
                for (int i = 0; i < 1000; ++i)
                    std::cout << "*";
                std::cout << std::endl;

                steady_clock::time_point t2 = steady_clock::now();

                duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

                std::cout << "It took me " << time_span.count() << " seconds.";
                std::cout << std::endl;
            }

            // 获取系统当前时间

            uint64_t TimeMilliSecond()
            {
                timeb now;
                ftime(&now);
                return now.time * 1000 + now.millitm;
            }

            uint64_t TimeMilliSecond1()
            {
                FILETIME file_time;
                GetSystemTimeAsFileTime(&file_time);
                uint64_t time = ((uint64_t)file_time.dwLowDateTime) + ((uint64_t)file_time.dwHighDateTime << 32);

                // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
                // until 00:00:00 January 1, 1970
                static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

                return (uint64_t)((time - EPOCH) / 10000LL);
            }

            /**
             * @brief 实现类似matlab的findpeaks功能。
             * http://cn.mathworks.com/help/signal/ref/findpeaks.html#buhdm28-1
             *
             * src: 输入数据
             * minPeakDistance: 相邻两个局部极大值的最小间距。
             * minHeightDiff: kernelSize范围内极大值与极小值的最小差值。
             *                与matlab的threshold目的一样，用来剔除flat peaks。
             * minPeakHeight：Peak的值低于minPeakHeight会被排除。
             */
            int findLocalMaximasLoc(const std::vector<double> &src, std::vector<int> &output_indices,
                            const int minPeakDistance, const double minHeightDiff, const double minPeakHeight)
            {
                cv::Mat m(src);
                return findLocalMaximasLoc(m, output_indices, minPeakDistance, minHeightDiff, minPeakHeight);
            }


            int findLocalMaximasLoc(const cv::Mat &src, std::vector<int> &output_indices,
                            const int minPeakDistance, const double minHeightDiff, const double minPeakHeight)
            {
                int kernelSize = minPeakDistance; // kernelSize指局部区域半径

                cv::Mat src_exp;
                cv::copyMakeBorder(src, src_exp, kernelSize, kernelSize, 0, 0,
                                   cv::BORDER_CONSTANT, cv::Scalar(0));

                int minLoc = 0;
                double minVal = 0, maxVal = 0; //, diff = 0;
                cv::Point minLocPt, maxLocPt;

                bool find_LM = false; // means not find localMaxima

                for (size_t loc = kernelSize; loc < src_exp.rows - kernelSize; loc++)
                // for (size_t loc = 0; loc < src.rows - 1; loc++)
                {
                    int start = loc - kernelSize;                    
                    int end = loc + kernelSize;
                    
                    cv::Mat m_part = src_exp.rowRange(start, end); //获取[Loc-kernelSize,Loc+kernelSize]范围内数据
                    // std::cout<<m_part.t()<<std::endl;
                    cv::minMaxLoc(m_part, &minVal, &maxVal, &minLocPt, &maxLocPt); //计算极大值位置

                    // maxLocPt.y == kernelSize,极大值应该在kernel区域中心
                    //使用minHeightDiff来剔除flat peaks
                    if (maxLocPt.y == kernelSize && maxVal > minPeakHeight &&
                        maxVal - minVal > minHeightDiff ) // && maxVal - minVal > diff)
                    {
                        find_LM = true;
                        // *maxLoc = loc - kernelSize + maxLocPt.y;
                        // diff = maxVal - minVal;
                        output_indices.push_back(loc-kernelSize);
                    }
                }
                return find_LM;
            }

            int findLocalMaximas(const cv::Mat &src, std::vector< std::pair<int, double> > &output_peaks,
                            const int minPeakDistance, const double minHeightDiff, const double minPeakHeight)
            {
                int kernelSize = minPeakDistance; // kernelSize指局部区域半径

                cv::Mat src_exp;
                cv::copyMakeBorder(src, src_exp, kernelSize, kernelSize, 0, 0, cv::BORDER_CONSTANT, cv::Scalar(0));


                int minLoc = 0;
                double minVal = 0, maxVal = 0; //, diff = 0;
                cv::Point minLocPt, maxLocPt;

                
                for (size_t loc = kernelSize; loc < src_exp.rows - kernelSize; loc++)
                {
                    cv::Mat m_part = src_exp.rowRange(loc - kernelSize, loc + kernelSize); //获取[Loc-kernelSize,Loc+kernelSize]范围内数据
                    cv::minMaxLoc(m_part, &minVal, &maxVal, &minLocPt, &maxLocPt);     //计算极大值位置
                    // std::cout << m_part.t() << std::endl;
                    // maxLocPt.y == kernelSize,极大值应该在kernel区域中心
                    //使用minHeightDiff来剔除flat peaks
                    if (maxLocPt.y == kernelSize && maxVal > minPeakHeight &&
                        maxVal - minVal > minHeightDiff) // && maxVal - minVal > diff)
                    {

                        // *maxLoc = loc - kernelSize + maxLocPt.y;
                        // diff = maxVal - minVal;
                        double gray;
                        if (src_exp.type() == CV_32F)
                            gray = src_exp.at<float>(loc, 0);
                        else if (src_exp.type() == CV_64F)
                            gray = src_exp.at<double>(loc, 0);
                        else
                        {
                            std::cerr << "local maxima input mat must bu 32FC1/64FC1" << std::endl;
                            return false;
                        }
                        int pos = loc - kernelSize;
                        output_peaks.push_back(std::make_pair(pos, gray));

                    }
                }

                if(output_peaks.empty())
                    return false;
                
                std::sort(output_peaks.begin(), output_peaks.end(), [&](std::pair<int, double> a, std::pair<int, double> b)
                      { return a.second > b.second; });
                
                return true;
            }



            bool IsSafeRect(const cv::Mat &image, cv::Rect rc)
            {
                if (rc.tl().x < 0)
                    return false;
                if (rc.tl().y < 0)
                    return false;
                if (rc.br().x > image.cols)
                    return false;
                if (rc.br().y > image.rows)
                    return false;

                return true;
            }

            cv::Rect getSafeRect(cv::Rect &rc, const cv::Mat &image)
            {
                int l, t, r, b;
                l = rc.x;
                t = rc.y;
                r = rc.br().x;
                b = rc.br().y;

                if (l < 0)
                    l = 0;
                if (t < 0)
                    t = 0;
                if (r > image.cols - 1)
                    r = image.cols - 1;
                if (b > image.rows - 1)
                    b = image.rows - 1;

                return cv::Rect(cv::Point(l, t), cv::Point(r, b));
            }

            cv::Rect getSafeRect(cv::Rect &rc, const cv::Size &size)
            {
                int l, t, r, b;
                l = rc.x;
                t = rc.y;
                r = rc.br().x;
                b = rc.br().y;

                if (l < 0)
                    l = 0;
                if (t < 0)
                    t = 0;
                if (r > size.width - 1)
                    r = size.width - 1;
                if (b > size.height - 1)
                    b = size.height - 1;

                return cv::Rect(cv::Point(l, t), cv::Point(r, b));
            }


            cv::Rect getSafeRectSizeFixed(cv::Rect &rc, const cv::Mat &image)
            {
                int width = rc.width;
                int height = rc.height;

                int l, t, r, b;
                l = rc.x;
                t = rc.y;
                r = rc.br().x;
                b = rc.br().y;

                if (l < 0)
                {
                    l = 0;
                    r = l + width - 1;
                }
                if (t < 0)
                {
                    t = 0;
                    b = t + height - 1;
                }
                if (r > image.cols - 1)
                {
                    r = image.cols - 1;
                    l = r - width + 1;
                }
                if (b > image.rows - 1)
                {
                    b = image.rows - 1;
                    t = b - height + 1;
                }
                return cv::Rect(cv::Point(l, t), cv::Point(r, b));
            }

            cv::Rect AdjustToSafeRect(const cv::Mat &image, cv::Rect rc)
            {
                int left, top, right, bottom;
                left = rc.x;
                top = rc.y;
                right = rc.x + rc.width - 1;
                bottom = rc.y + rc.height - 1;

                if (left < 0)
                    left = 0;
                if (top < 0)
                    top = 0;
                if (right > image.cols - 1)
                    right = image.cols - 1;
                if (bottom > image.rows - 1)
                    bottom = image.rows - 1;

                return cv::Rect(left, top, right - left, bottom - top);
            }

            

            double distance(cv::Point point, cv::Size size)
            {
                return sqrt(pow((point.x - size.height / 2), 2) + pow((point.y - size.width / 2), 2));
            }

            double maxValue(double a, double b)
            {
                return a > b ? a : b;
            }

           

            int parseArgsByLines(std::string args_file_path, std::vector<std::string> &args)
            {
                // parse args
                std::ifstream ifs(args_file_path, std::ios::in);
                if (!ifs.is_open())
                {
                    std::cout << "Failed to read args file!" << std::endl;
                    return -1;
                }

                std::string linetext;
                while (getline(ifs, linetext))
                {
                    args.push_back(linetext);
                    std::cout << linetext << std::endl;
                }
                ifs.close();
                return 0;
            }
        
            std::string to_string(double val, int fixed)
            {
                std::string str = std::to_string(val);
                return str.substr(0, str.find(".") + fixed + 1);
            }

            std::string to_string(int val)
            {
                std::string str = std::to_string(val);
                return str.substr(0, str.find("."));
            }

            std::string toLower(std::string src)
            {
                std::string dst = src;
                std::transform(dst.begin(), dst.end(), dst.begin(),
                               [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                return dst;
            }

            std::string toUpper(std::string src)
            {
                std::string dst = src;
                std::transform(dst.begin(), dst.end(), dst.begin(),
                               [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
                return dst;
            }

            //根据first的值降序排序
            bool cmp1(std::pair<int, int> a, std::pair<int, int> b)
            {
                return a.first > b.first;
            }

            //根据second的值降序排序
            bool cmp2(std::pair<int, int> a, std::pair<int, int> b)
            {
                return a.second > b.second;
            }

            //注意这两个函数是否有内存访问越界，heap或stack崩溃
            std::wstring StringToWString(const std::string &str)
            {
                int num = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
                wchar_t *wide = new wchar_t[num];
                MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide, num);
                std::wstring w_str(wide);
                delete[] wide;
                return w_str;
            }

            std::string WStringToString(const std::wstring &wstr)
            {
                std::string str;
                int nLen = (int)wstr.length();
                str.resize(nLen, ' ');
                int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), nLen, (LPSTR)str.c_str(), nLen, NULL, NULL);
                if (nResult == 0)
                {
                    return "";
                }
                return str;
            }

            void TrimString(std::string &str)
            {
                int s = str.find_first_not_of(" ");
                int e = str.find_last_not_of(" ");
                str = str.substr(s, e - s + 1);
                return;
            }

            bool SplitString(const std::string &src, const std::string &flag, std::vector<std::string> &dst)
            {
                using namespace std;
                dst.clear();
                if (src.empty() || flag.empty())
                {
                    //	cerr << "[MatchResultIndexFileGenerater:: SplitString] Invalid input! " << endl;
                    return false;
                }

                if (src.find(flag) == string::npos)
                {
                    return false;
                }

                string sub_src = src;
                int flag_len = flag.size();
                while (!sub_src.empty())
                {
                    int index = sub_src.find_first_of(flag);

                    if (index == -1)
                    {
                        dst.push_back(sub_src);
                        break;
                    }

                    if (index != 0)
                    {
                        dst.push_back(sub_src.substr(0, index));
                    }

                    if (index + flag_len != sub_src.size())
                    {
                        sub_src = sub_src.substr(index + flag_len, sub_src.size() - index);
                    }
                    else
                        sub_src = "";
                }
                return true;

                
            }

            double MinValue()
            {
                return  (std::numeric_limits<double>::min)();
                
            }

            double Epsilon()
            {
                return (std::numeric_limits<double>::epsilon)();
            }




        
        }

        // end of namespace of utilities
    }
}
