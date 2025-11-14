#pragma once

#include <memory>
#include <vector>

#include "core/vec3.h"
#include "core/ray.h"

#include "geom/hittable.h"

// Forward declare to avoid circular include
class Material;

namespace rt::integrator {

// Result of intersection (GPU/CPU integrator fills this)
struct HitInfo {
    bool hit = false;
    float t = 0;
    core::Point3 p;
    core::Vec3 normal;

    double u = 0.0;
    double v = 0.0;
    bool front_face = true;

    const std::shared_ptr<Material> mat;
};

class RayIntegrator {
public:
    // Batch intersection API (CPU or GPU)
    virtual void IntersectBatch(
        const std::vector<core::Ray>& rays,
        std::vector<geom::HitRecord>& hits
    ) const = 0;
};

} // namespace rt::integrator
