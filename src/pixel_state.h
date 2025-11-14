#pragma once
#include "color.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// keeps track of pixel state for wavefront renderer
// stoes data necessary for adaptive sampling logic
struct PixelState {
    color sum = color(0,0,0);
    color mean = color(0,0,0); // running average
    color m2 = color(0,0,0);   // sum of squared differences (for variance)
    int   samples = 0;
    bool  converged = false;
};

// called whenever ray is terminated
inline void record_sample(PixelState& ps, const color& sample) {
    ps.samples++;

    // welfords algorithm for updating variance efficiently
    for(int c = 0; c < 3; ++c) {
        double x = sample[c];
        double mu = ps.mean[c];

        double delta = x - mu;
        mu += delta / ps.samples;
        double delta2 = x - mu;

        ps.mean[c] = mu;
        ps.m2[c] += delta2 * delta;
    }

    ps.sum += sample;
}

inline color variance(const PixelState& ps) {
    color var(0,0,0);
    if( ps.samples > 1 ) {
        for(int c = 0; c < 3; ++c) {
            var[c] = ps.m2[c] / (ps.samples - 1);
        }
    }
    return var;
}

// adaptive sampling convergence test
// rel_threshold is the relative threshold (0.02 is good)
// min_spp is minimum samples (16 is good)
inline bool is_converged(PixelState& ps, double rel_threshold, int min_spp) {
    if( ps.samples < min_spp ) return false;

    color var = variance(ps);
    bool ok = true;

    for(int c= 0; c < 3; ++c) {
        double mu = std::max(std::abs(ps.mean[c]), 1e-3);
        double sigma = std::sqrt(var[c]);
        double err = sigma / std::sqrt(ps.samples);

        if( err / mu > rel_threshold ) {
            ok = false;
            break;
        }
    }

    return ok;
}
