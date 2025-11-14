#pragma once

#include "core/ray.h"
#include "core/color.h"

namespace rt::integrator {

struct RayState {
    core::Ray   r;
    int   pixel_index = 0;
    int   depth = 0;
    core::Color throughput = core::Color(1,1,1);
};

} // namespace rt::integrator
