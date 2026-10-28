// dip_demos_2025a.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>

#include "2025A/homework.h"

// argc: arguments count,  3
// argv: arguments value,  dip_demos.exe d:\\Works\\cameraman.bmp  15


int main(int argc, char *argv[])
{
    std::cout << "Start digital image processing 2025A..." << std::endl;
    std::cout << std::endl; 

    if (argc < 4)
    {
        std::cout << "Error: Please input directory, image filename and kernel filename!" << std::endl;
        return -1;
    }
    std::cout << "arguments count:" << argc << std::endl;
    for (int arg_index = 0; arg_index < argc; arg_index++)
        std::cout << "argv[" << arg_index << "]: " << argv[arg_index] << std::endl;
    std::cout << std::endl;
    
    char* dir = argv[1];
    char* image_name = argv[2];
    
    // DIPAutumn2025::grayTrans(dir, image_name);
    
    // DIPAutumn2025::geoTrans(dir, image_name, argv[3]);
    
    DIPAutumn2025::conv2d(dir, image_name, argv[3]);
    // DIPAutumn2025::conv2dSeparated(dir, image_name, argv[4]);

    // DIPAutumn2025::canny(dir, image_name, atof(argv[5]), atof(argv[6]), atoi(argv[7]));
    // DIPAutumn2025::harrisCorners(dir, image_name);
    // DIPAutumn2025::siftCorners(dir, image_name);
    // DIPAutumn2025::siftMatch(argv[1], argv[2], argv[3]);

    return 0;   
}