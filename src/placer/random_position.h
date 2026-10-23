#pragma once
#include "string"
#include "vector"

#include "data_types.h"

struct RandomPosition {
    CVector3f position;
    CVector3f aimPosition;
    std::string name;
    std::string region;
    std::string type;
};