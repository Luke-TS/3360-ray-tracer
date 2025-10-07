#pragma once

#include "camera.h"
#include "color.h"
#include "scene.h"
#include "ray.h"

#include <cmath>
#include <math.h>

class Sampler {
public:
    virtual ~Sampler() = default;

    // return number of samples taken
    virtual int sample_pixel(color& pixel, const Scene& world, const camera& cam, int i, int j) const = 0;
};

class DefaultSampler : public Sampler {
public:
    DefaultSampler(int num_samples) : num_samples(num_samples) {}

    int sample_pixel(color& pixel, const Scene& world, const camera& cam, int i, int j) const override {
        pixel = color(0,0,0);
        for(int k = 0; k < num_samples; k++) {
            ray r = cam.get_ray(i, j);
            pixel += cam.get_pixel(r, cam.max_depth, world); 
        }
        pixel /= num_samples;
        return num_samples;
    }
private:
    int num_samples;
};

// threshold values
// preview 0.02 - 0.05
// good    0.005 - 0.01
// final   0.001 - 0.003
class AdaptiveSampler : public Sampler {
public:
    AdaptiveSampler(int min_samples, int max_samples, float threshold) : min_samples(min_samples), max_samples(max_samples), threshold(threshold) {}

    int sample_pixel(color& pixel, const Scene& world, const camera& cam, int i, int j) const override {
        pixel = color(0,0,0);
        color sum   = color(0,0,0);
        color sum_sq= color(0,0,0);
        int samples = 0;
        long total_samples = 0;

        while( samples <= max_samples ) {
            samples++;
            ray r = cam.get_ray(i, j);
            pixel += cam.get_pixel(r, cam.max_depth, world); 

            sum += pixel;
            sum_sq += pixel * pixel;

            if( samples >= min_samples ) {
                color mean = sum / samples;
                double mean_luminance = luminance(mean);

                color variance = (sum_sq / samples) - (mean * mean);
                double error = sqrt(luminance(variance) / samples);

                // using relative error
                if( (error / (mean_luminance + 1e-3f)) < threshold ) {
                    break;
                }
            }
        }
        pixel /= samples;
        return samples;
    }

private:
    int min_samples;
    int max_samples;
    float threshold;
};
