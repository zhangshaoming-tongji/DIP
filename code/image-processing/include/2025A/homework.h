#ifndef _CVRSG_KONGZI_HOMEWORK_2025_AUTUMN_H
#define _CVRSG_KONGZI_HOMEWORK_2025_AUTUMN_H

namespace DIPAutumn2025
{
    int grayTrans(char* dir, char* image_name);
    int geoTrans(char* dir, char* image_name, char* params_filename);

    int conv2d(char* dir, char* image_name, char* kernel_name);
    int conv2dSeparated(char* dir, char* image_name, char* kernel_name);
}

#endif //_CVRSG_KONGZI_HOMEWORK_2025_AUTUMN_H