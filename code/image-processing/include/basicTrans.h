#ifndef _CVRSG_KONGZI_BASIC_TRANS_H
#define _CVRSG_KONGZI_BASIC_TRANS_H

#include <string>

namespace cvrsg
{
    namespace kongzi
    {
        namespace transform
        {

            //课堂练习
            //练习1：直方图均衡化
            int ImageHistEqualExercise(std::string path);
                      
            //练习2：使用仿射变换进行图像变换
            int WarpImageUinsgAffineExercise(std::string file_path,
                double angle_deg = 0.0,
                double trans_x = 0.0,
                double trans_y = 0.0,
                double scale_x = 1.0,
                double scale_y = 1.0);
            
            //练习3：计算仿射变换并进行图像变换
            int CalcTransfromAndWarpExercise(std::string file_path);

            
            //参考代码=========================================
            //练习4：透视变换 用于图像变换 逆变换 图像变形
            int CalcTransfromAndWarp(std::string file_path, 
                int pts_number,
                int* pts_src_x, 
                int* pts_src_y, 
                int* pts_dst_x, 
                int* pts_dst_y);
        
            //参考代码：直方图均衡化 用于图像增强
            int HistEqual(std::string path);

            //参考代码：计算仿射变换 用于图像变换 图像变形
            int CalcAffineTransform(std::string file_path);

            //参考代码：旋转、缩放、平移
            int RotateScaleTranslationExercise(std::string file_path,
                double angle_deg = 0.0,
                double trans_x = 0.0,
                double trans_y = 0.0,
                double scale_x = 1.0,
                double scale_y = 1.0);

            
        }
    }
}

#endif //_CVRSG_KONGZI_BASIC_TRANS_H