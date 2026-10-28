#ifndef _CVRSG_KONGZI_BASIC_TRANS_2024_AUTUMN_H
#define _CVRSG_KONGZI_BASIC_TRANS_2024_AUTUMN_H

#include <string>

namespace graytrans2024A
{
    int calcHist(std::string);
    enum GrayTransType
    {
        LOG = 0,
        EXP,
        EQUALIZEHIST

    };
    int grayTransfrom(std::string dir, std::string name, int method_type, float param = 1.0);

    int grayTrans(std::string dir, std::string name, float start);


}

#endif