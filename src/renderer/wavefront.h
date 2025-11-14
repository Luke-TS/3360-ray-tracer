#pragma once

#include <vector>
#include <iostream>
#include <algorithm>

#include "core/color.h"
#include "integrator/pixel_state.h"
#include "integrator/ray_state.h"

namespace rt::scene {
class Scene;
class Camera;
}

namespace rt::integrator {
class RayIntegrator;
}

namespace rt::renderer {

class WavefrontRenderer {
public:
    WavefrontRenderer(
        const scene::Scene&      world,
        const scene::Camera&     cam,
        integrator::RayIntegrator& integrator,
        int max_depth   = 10,
        int max_samples = 128,
        int batch_size  = 8192
    );

    void Render();   // Only declaration here

private:
    const scene::Scene&   world;
    const scene::Camera&  cam;
    integrator::RayIntegrator& integrator;

    int max_depth;
    int max_ssp;
    int batch_size;

    static rt::core::Color background(const rt::core::Ray& r);
};

} // namespace rt::renderer
