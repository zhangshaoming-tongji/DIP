#include "Drawing.h"

#include <io.h>
#include <direct.h>

#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <ratio>
#include <chrono>

#include "utilities.h"

#define MAX_PATH 260

namespace cvrsg
{
    namespace common
    {

        void Drawing::PutText(cv::Mat &image, cv::Point pos, std::string text,
                              cv::Scalar color, float scale, int thickness, bool fill_bkg)
        {
            // std::string text = "Lambda1 <= 0";
            int baseline;
            cv::Size text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, scale, thickness, &baseline);

            int height = text_size.height + 6;

            if (fill_bkg)
            {
                cv::Rect rc_bkg(pos + cv::Point(0,10-2), cv::Size(text_size.width, height));
                cv::rectangle(image, rc_bkg, cv::Scalar(0, 0, 0), -1);
            }
            pos += cv::Point(0, text_size.height + 10);
            cv::putText(image, text, pos,
                        cv::FONT_HERSHEY_SIMPLEX, scale, color, thickness);
        }

        void Drawing::DrawRotatedRect(const cv::Mat &image, cv::RotatedRect rrc, cv::Scalar color, int thickness)
        {
            using namespace cv;
            using namespace std;
            Point2f vertices[4];
            rrc.points(vertices);
            for (int i = 0; i < 4; i++)
                line(image, vertices[i], vertices[(i + 1) % 4], color, thickness);
            // Rect brect = rrc.boundingRect();
            // rectangle(image, brect, color);
        }

        void Drawing::DrawHist(const cv::Mat &image, cv::Mat &hist_image, int histWidth, int histHeight)
        {
            if (histWidth < 280)
                histWidth = 280;

            cv::Mat hist;

            // bins of histogram
            int histSize = 256;
            // range of pixel gray ( R,G,B) )
            float range[] = {0, 256};
            const float *histRange = {range};
            bool uniform = true;
            bool accumulate = false;

            cv::calcHist(&image, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

            int type = hist.type();
            // for (int i = 0; i <256; i++)
            //	std::cout << i << ":  " << hist.at<float>(i) << std::endl;

            int binWidth = cvRound((double)histWidth / histSize);
            hist_image = cv::Mat(histHeight, histWidth, CV_8UC3, cv::Scalar(0, 0, 0));

            // [ 0, histImage.rows ]
            cv::normalize(hist, hist, 0, hist_image.rows, cv::NORM_MINMAX, -1, cv::Mat());

            // draw data of histogram
            for (int i = 1; i < histSize; i++)
            {
                cv::line(hist_image,
                         cv::Point(binWidth * (i - 1), histHeight - cvRound(hist.at<float>(i - 1))),
                         cv::Point(binWidth * (i), histHeight - cvRound(hist.at<float>(i))),
                         cv::Scalar(255, 255, 255), 1, 8, 0);

                cv::line(hist_image,
                         cv::Point(binWidth * (i), histHeight),
                         cv::Point(binWidth * (i), histHeight - cvRound(hist.at<float>(i))),
                         cv::Scalar(0, 255, 0), 1, 8, 0);
            }
        }

        void Drawing::DrawAndShowHist(const cv::Mat &image, std::string windowName, int histWidth, int histHeight)
        {
            if (histWidth < 280)
                histWidth = 280;

            cv::Mat hist;

            // bins of histogram
            int histSize = 256;
            // range of pixel gray ( R,G,B) )
            float range[] = {0, 256};
            const float *histRange = {range};
            bool uniform = true;
            bool accumulate = false;

            cv::calcHist(&image, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

            int type = hist.type();
            // for (int i = 0; i <256; i++)
            //	std::cout << i << ":  " << hist.at<float>(i) << std::endl;

            int binWidth = cvRound((double)histWidth / histSize);
            cv::Mat histImage(histHeight, histWidth, CV_8UC3, cv::Scalar(0, 0, 0));

            // [ 0, histImage.rows ]
            cv::normalize(hist, hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());

            // draw data of histogram
            for (int i = 1; i < histSize; i++)
            {
                cv::line(histImage,
                         cv::Point(binWidth * (i - 1), histHeight - cvRound(hist.at<float>(i - 1))),
                         cv::Point(binWidth * (i), histHeight - cvRound(hist.at<float>(i))),
                         cv::Scalar(255, 255, 255), 1, 8, 0);

                cv::line(histImage,
                         cv::Point(binWidth * (i), histHeight),
                         cv::Point(binWidth * (i), histHeight - cvRound(hist.at<float>(i))),
                         cv::Scalar(0, 255, 0), 1, 8, 0);
            }

            cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
            imshow(windowName, histImage);

            cv::waitKey(0);
        }

        void Drawing::VisualizeMatrix(const cv::Mat &src, std::string window_name, float alpha)
        {
            cv::Mat dst;
            src.convertTo(dst, CV_32F);
            cv::normalize(src, dst, 0, 255, cv::NORM_MINMAX);

            double min, max;
            cv::minMaxIdx(dst, &min, &max);
            cv::convertScaleAbs(dst, dst, 255 / max);

            cv::Mat mat_color;
            cv::applyColorMap(dst, mat_color, cv::COLORMAP_RAINBOW);

            cv::resize(mat_color, mat_color, cv::Size(mat_color.cols * alpha, mat_color.rows * alpha));

            cv::imshow(window_name, mat_color);
            cv::waitKey(0);
        }

        void Drawing::DrawHist(const cv::Mat &src_hist, cv::Mat &histImage,
                               const std::vector<int>& maxima_indices, cv::Scalar color,
                               int margin, int xTickStep, int yTickStep)
        {
            int histSize = src_hist.rows;
            int width = histSize * 4;
            int height = width * 0.618;

            int binWidth = width / (float)histSize;

            double maxv;
            cv::minMaxLoc(src_hist, NULL, &maxv);

            if (histImage.empty())
            {
                histImage = cv::Mat(height + margin * 2, width + margin * 2, CV_8UC3, cv::Scalar(0, 0, 0));

                cv::Point origin(margin, histImage.rows - margin);
                origin.x -= 1;
                origin.y += 1;
                cv::Point xAxisEnd(histImage.cols - margin, origin.y);
                cv::Point yAxisEnd(origin.x, margin);

                cv::line(histImage, origin, xAxisEnd, cv::Scalar(255, 255, 255), 1);
                cv::line(histImage, origin, yAxisEnd, cv::Scalar(255, 255, 255), 1);

                std::string upper_str = utilities::to_string(maxv,2);
                cv::putText(histImage, upper_str, cv::Point(margin / 2 - 7, yAxisEnd.y), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
                int baseline;
                std::string upper_half_str = std::to_string((int)(maxv/2));
                cv::Size textSize = cv::getTextSize(upper_half_str, cv::FONT_HERSHEY_PLAIN, 1, 1, &baseline);
                cv::putText(histImage, upper_half_str, cv::Point(margin - 2 - textSize.width, yAxisEnd.y + height / 2), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
                cv::putText(histImage, std::to_string(histSize), cv::Point(xAxisEnd.x, xAxisEnd.y + margin / 2), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
                cv::putText(histImage, "0", cv::Point(origin.x, origin.y + margin / 2), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));

                for (int i = xTickStep; i < histSize; i += xTickStep)
                {
                    cv::Point ptTick(origin.x + i * binWidth, origin.y + margin / 2);
                    textSize = cv::getTextSize(std::to_string(i), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseline);
                    cv::putText(histImage, std::to_string(i),
                                cv::Point(ptTick.x - textSize.width / 2, ptTick.y), 
                                cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));

                    cv::line(histImage, cv::Point(ptTick.x, origin.y),
                             cv::Point(ptTick.x, origin.y + 3), cv::Scalar(255, 255, 255), 1);
                }
            }

            cv::Mat hist;
            cv::normalize(src_hist, hist, 0, 1, cv::NORM_MINMAX);
            

            for (int i = 1; i < histSize; i++)
            {
                float histValuePrev = hist.at<float>(i - 1);
                float histValue = hist.at<float>(i);

                cv::Point pt1 = cv::Point(margin + (i - 1) * binWidth, margin + height - histValuePrev * height);
                cv::Point pt2 = cv::Point(margin + i * binWidth, margin + height - histValue * height);

                // cv::Point pt3 = cv::Point(      margin + i * bin_w, histImage.rows - margin);

                line(histImage, pt1, pt2, color, 1, cv::LINE_AA);
                // line(histImage, pt3,pt2, color, 1);//, cv::LINE_AA);
            }

            if(maxima_indices.empty())
                return;

            for (int i = 0; i < maxima_indices.size(); i++)
            {
                int index = maxima_indices[i];
                float histValue = hist.at<float>(index,0);

                cv::Point peak = cv::Point(margin +  index * binWidth, margin + height - histValue * height);    
                cv::circle(histImage, peak, 2, cv::Scalar(0,0,255),2);
               
            }
        }
    
        void Drawing::DrawHist(const cv::Mat &src_hist, cv::Mat &histImage,
                               const std::vector< std::pair<int,double> >& maximas, cv::Scalar color,
                               int margin, int xTickStep, int yTickStep)
        {
            int histSize = src_hist.rows;
            int width = histSize * 4;
            int height = width * 0.618;

            int binWidth = width / (float)histSize;

            double maxv;
            cv::minMaxLoc(src_hist, NULL, &maxv);

            if (histImage.empty())
            {
                histImage = cv::Mat(height + margin * 2, width + margin * 2, CV_8UC3, cv::Scalar(0, 0, 0));

                cv::Point origin(margin, histImage.rows - margin);
                origin.x -= 1;
                origin.y += 1;
                cv::Point xAxisEnd(histImage.cols - margin, origin.y);
                cv::Point yAxisEnd(origin.x, margin);

                cv::line(histImage, origin, xAxisEnd, cv::Scalar(255, 255, 255), 1);
                cv::line(histImage, origin, yAxisEnd, cv::Scalar(255, 255, 255), 1);

                std::string upper_str = utilities::to_string(maxv,2);
                cv::putText(histImage, upper_str, cv::Point(margin / 2 - 7, yAxisEnd.y), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
                
                int baseline;
                std::string upper_half_str = utilities::to_string(maxv/2,2);
                cv::Size textSize = cv::getTextSize(upper_half_str, cv::FONT_HERSHEY_PLAIN, 1, 1, &baseline);
                
                cv::putText(histImage, upper_half_str, cv::Point(margin - 2 - textSize.width, yAxisEnd.y + height / 2), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
                cv::putText(histImage, std::to_string(histSize), cv::Point(xAxisEnd.x, xAxisEnd.y + margin / 2), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
                cv::putText(histImage, "0", cv::Point(origin.x, origin.y + margin / 2), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));

                for (int i = xTickStep; i < histSize; i += xTickStep)
                {
                    cv::Point ptTick(origin.x + i * binWidth, origin.y + margin / 2);
                    textSize = cv::getTextSize(std::to_string(i), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseline);
                    cv::putText(histImage, std::to_string(i),
                                cv::Point(ptTick.x - textSize.width / 2, ptTick.y), 
                                cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));

                    cv::line(histImage, cv::Point(ptTick.x, origin.y),
                             cv::Point(ptTick.x, origin.y + 3), cv::Scalar(255, 255, 255), 1);
                }
            }

            cv::Mat hist;
            cv::normalize(src_hist, hist, 0, 1, cv::NORM_MINMAX);
            

            for (int i = 1; i < histSize; i++)
            {
                float histValuePrev = hist.at<float>(i - 1);
                float histValue = hist.at<float>(i);

                cv::Point pt1 = cv::Point(margin + (i - 1) * binWidth, margin + height - histValuePrev * height);
                cv::Point pt2 = cv::Point(margin + i * binWidth, margin + height - histValue * height);

                // cv::Point pt3 = cv::Point(      margin + i * bin_w, histImage.rows - margin);

                line(histImage, pt1, pt2, color, 1, cv::LINE_AA);
                // line(histImage, pt3,pt2, color, 1);//, cv::LINE_AA);
            }

            if(maximas.empty())
                return;

            double min_peak = maximas.back().second;
            double max_peak = maximas.front().second;

            for (int i = 0; i < maximas.size(); i++)
            {
                int index = maximas[i].first;
                double value = maximas[i].second;

                float color_pos = (value-min_peak) / (max_peak-min_peak);
                cv::Scalar peak_color((1-color_pos)*255, 0, color_pos * 255) ;

                float histValue = hist.at<float>(index,0);

                cv::Point peak = cv::Point(margin +  index * binWidth, margin + height - histValue * height); 

                if(i ==0)   
                    cv::circle(histImage, peak, 4, peak_color,2);
                else
                    cv::circle(histImage, peak, 2, peak_color,2);
               
            }
        }

        void Drawing::DrawHistBars(const cv::Mat &src_hist, cv::Mat &histImage,
                               const std::vector< std::pair<int,double> >& maximas, int dst_width,cv::Scalar color,
                               int margin, int xTickStep, int yTickStep)
        {
            int histSize = src_hist.rows;
            
            int width = dst_width;
            if(width <= 0)
                width = histSize * 4;
            
            int height = width * 0.618;

            int binWidth = width / (float)histSize;

            double maxv;
            cv::minMaxLoc(src_hist, NULL, &maxv);

            if (histImage.empty())
            {
                histImage = cv::Mat(height + margin * 2, width + margin * 2, CV_8UC3, cv::Scalar(0, 0, 0));

                cv::Point origin(margin, histImage.rows - margin);
                origin.x -= 1;
                origin.y += 1;
                cv::Point xAxisEnd(histImage.cols - margin, origin.y);
                cv::Point yAxisEnd(origin.x, margin);

                cv::line(histImage, origin, xAxisEnd, cv::Scalar(255, 255, 255), 1);
                cv::line(histImage, origin, yAxisEnd, cv::Scalar(255, 255, 255), 1);

                std::string upper_str = utilities::to_string(maxv,2);
                cv::putText(histImage, upper_str, cv::Point(margin / 2 - 7, yAxisEnd.y), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
                
                int baseline;
                std::string upper_half_str = utilities::to_string(maxv/2,2);
                cv::Size textSize = cv::getTextSize(upper_half_str, cv::FONT_HERSHEY_PLAIN, 1, 1, &baseline);
                
                cv::putText(histImage, upper_half_str, cv::Point(margin - 2 - textSize.width, yAxisEnd.y + height / 2), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
                cv::putText(histImage, std::to_string(histSize), cv::Point(xAxisEnd.x, xAxisEnd.y + margin / 2), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
                cv::putText(histImage, "0", cv::Point(origin.x, origin.y + margin / 2), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));

                for (int i = xTickStep; i < histSize; i += xTickStep)
                {
                    cv::Point ptTick(origin.x + i * binWidth, origin.y + margin / 2);
                    textSize = cv::getTextSize(std::to_string(i), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseline);
                    cv::putText(histImage, std::to_string(i),
                                cv::Point(ptTick.x - textSize.width / 2, ptTick.y), 
                                cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));

                    cv::line(histImage, cv::Point(ptTick.x, origin.y),
                             cv::Point(ptTick.x, origin.y + 3), cv::Scalar(255, 255, 255), 1);
                }
            }

            cv::Mat hist;
            cv::normalize(src_hist, hist, 1, 0, cv::NORM_L2);
            

            for (int i = 0; i < histSize; i++)
            {
                float histValue = hist.at<float>(i);

                cv::Point pt_zero  = cv::Point(margin + i * binWidth, margin + height );
                cv::Point pt_value = cv::Point(margin + i * binWidth, margin + height - histValue * height);

                // cv::Point pt3 = cv::Point(      margin + i * bin_w, histImage.rows - margin);

                line(histImage, pt_zero, pt_value, color, 1, cv::LINE_AA);
                // line(histImage, pt3,pt2, color, 1);//, cv::LINE_AA);
            }

            if(maximas.empty())
                return;

            double min_peak = maximas.back().second;
            double max_peak = maximas.front().second;

            for (int i = 0; i < maximas.size(); i++)
            {
                int index = maximas[i].first;
                double value = maximas[i].second;

                float color_pos = (value-min_peak) / (max_peak-min_peak);
                cv::Scalar peak_color((1-color_pos)*255, 0, color_pos * 255) ;

                float histValue = hist.at<float>(index,0);

                cv::Point peak = cv::Point(margin +  index * binWidth, margin + height - histValue * height); 

                if(i ==0)   
                    cv::circle(histImage, peak, 4, peak_color,2);
                else
                    cv::circle(histImage, peak, 2, peak_color,2);
               
            }
        }



        void Drawing::DrawLOG(cv::Mat &image, std::string text, float scale, int thickness)
        {
            common::Drawing::PutText(image, cv::Point(10, image.rows - 40), text, cv::Scalar(0, 255, 255), scale, thickness);
            std::string time_text = common::utilities::getCurrentSystemTime();
            common::Drawing::PutText(image, cv::Point(image.cols - time_text.size()*10, image.rows - 40), time_text, cv::Scalar(0, 255, 255), scale, thickness);
        }
    }
}
