#pragma once

#include "camera.h"
#include "color.h"
#include "scene.h"
#include "ray.h"

class Sampler {
public:
    virtual ~Sampler() = default;

    virtual color sample_pixel(Scene& world, const camera& cam, int i, int j) = 0;
};

class DefaultSampler : public Sampler {
public:
    DefaultSampler(int num_samples) : num_samples(num_samples) {}

    color sample_pixel(Scene& world, const camera& cam, int i, int j) {
        color pixel = color(0,0,0);
        for(int k = 0; k < num_samples; k++) {
            ray r = cam.get_ray(i, j);
            pixel += cam.get_pixel(r, cam.max_depth, world); 
        }
        return pixel / num_samples;
    }
private:
    int num_samples;
};
