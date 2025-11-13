#pragma once
#include "ray.h"
#include "color.h"

struct RayState {
    ray   r;
    int   pixel_index = 0;
    int   depth = 0;
    color throughput = color(0,0,0);
};
