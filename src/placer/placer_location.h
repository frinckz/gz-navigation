#pragma once
#include "string"
#include "vector"

#include "data_types.h"

struct PlacerLocationMetadata {
    std::string name;
    std::string region;
    std::string type;
};

struct PlacerLocation {
    CVector3f position{};
    CVector3f aimPosition{};
    PlacerLocationMetadata locationMetadata;
};
