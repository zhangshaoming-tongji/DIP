#ifndef _CVRSG_KONGZI_BASIC_GEO_TRANS_2024_AUTUMN_H
#define _CVRSG_KONGZI_BASIC_GEO_TRANS_2024_AUTUMN_H

#include <string>
namespace geotrans2024A
{
    int geoTransViaAffineMatrix(std::string image_path, std::string matrix_path);
    int geoTransViaAngleScaleTrans(std::string image_path, std::string params_path);
    int geoTransViaPoints(std::string image_path, std::string points_path);
    
}


#endif //_CVRSG_KONGZI_BASIC_GEO_TRANS_2024_AUTUMN_H
