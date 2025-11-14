#pragma once

#include "core/color.h"
#include "core/ray.h"

#include "scene/camera.h"
#include "scene/scene.h"

#include <cmath>
#include <math.h>

namespace rt::integrator {

class Sampler {
public:
    virtual ~Sampler() = default;

    // return number of samples taken
    virtual int SamplePixel(core::Color& pixel, const scene::Scene& world, const scene::Camera& cam, int i, int j) const = 0;
};

class DefaultSampler : public Sampler {
public:
    DefaultSampler(int num_samples) : num_samples_(num_samples) {}

    int SamplePixel(core::Color& pixel, const scene::Scene& world, const scene::Camera& cam, int i, int j) const override {
        pixel = core::Color(0,0,0);
        for(int k = 0; k < num_samples_; k++) {
            core::Ray r = cam.GetRay(i, j);
            pixel += cam.GetPixel(r, cam.max_depth_, world); 
        }
        pixel /= num_samples_;
        return num_samples_;
    }
private:
    int num_samples_;
};

// threshold values
// preview 0.02 - 0.05
// good    0.005 - 0.01
// final   0.001 - 0.003
class AdaptiveSampler : public Sampler {
public:
    AdaptiveSampler(int min_samples, int max_samples, float threshold) : min_samples_(min_samples), max_samples_(max_samples), threshold_(threshold) {}

    int SamplePixel(core::Color& pixel, const scene::Scene& world, const scene::Camera& cam, int i, int j) const override {
        pixel = core::Color(0,0,0);
        core::Color sum   = core::Color(0,0,0);
        core::Color sum_sq= core::Color(0,0,0);
        int samples = 0;

        while( samples <= max_samples_ ) {
            samples++;
            core::Ray r = cam.GetRay(i, j);
            pixel += cam.GetPixel(r, cam.max_depth_, world); 

            sum += pixel;
            sum_sq += pixel * pixel;

            if( samples >= min_samples_ ) {
                core::Color mean = sum / samples;
                double mean_luminance = luminance(mean);

                core::Color variance = (sum_sq / samples) - (mean * mean);
                double error = sqrt(luminance(variance) / samples);

                // using relative error
                if( (error / (mean_luminance + 1e-3f)) < threshold_ ) {
                    break;
                }
            }
        }
        pixel /= samples;
        return samples;
    }

private:
    int min_samples_;
    int max_samples_;
    float threshold_;
};

} // namespace rt::integrator
