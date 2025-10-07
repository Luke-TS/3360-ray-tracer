#pragma once

#include "camera.h"
#include "color.h"
#include "scene.h"

class Sampler {
public:
    virtual color sample_pixel(Scene& world, const ray& r, const camera& cam);
};

class DefaultSampler : Sampler {
public:
    DefaultSampler(int depth) : depth(depth) {}


private:
    int depth;
};
