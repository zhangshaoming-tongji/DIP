#ifndef _CVRSG_KONGZI_CONVOLUTION_2024_AUTUMN_H
#define _CVRSG_KONGZI_CONVOLUTION_2024_AUTUMN_H

#include <string>

namespace conv2024A
{
	int conv2d(std::string image_path, std::string kernel_path);
	int conv2d_onclass(std::string image_path, std::string kernel_path);
		
	int conv2dSeparated(std::string image_path, std::string kernel_path);
	
	int conv2dSeparated_onclass(std::string image_path, std::string kernel_path);

	int medianFilter(std::string image_path);
	int medianFilter_onclass(std::string image_path);

}

#endif //_CVRSG_KONGZI_CONVOLUTION_2024_AUTUMN_H