#pragma once

#include "camera.h"
#include "color.h"
#include "hittable_list.h"
class Sampler {
public:
    virtual color sample_pixel(hittable_list& world, const ray& r, const camera& cam);
};

class DefaultSampler : Sampler {
public:
    DefaultSampler(int depth) : depth(depth) {}
private:
    int depth;
};
