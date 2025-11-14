#pragma once
#include "ray.h"
#include "color.h"

struct RayState {
    Ray   r;
    int   pixel_index = 0;
    int   depth = 0;
    Color throughput = Color(1,1,1);
};
