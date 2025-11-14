#pragma once

#include <vector>
#include "hittable.h"
#include "ray.h"
#include "scene.h"   // for point3, vec3, hit_record-compatible types

// Forward declare to avoid circular include
class Material;

// Result of intersection (GPU/CPU integrator fills this)
struct HitInfo {
    bool hit = false;
    float t = 0;
    Point3 p;
    Vec3 normal;

    double u = 0.0;
    double v = 0.0;
    bool front_face = true;

    const shared_ptr<Material> mat;   // pointer only, no need for full type
};

class RayIntegrator {
public:
    virtual ~RayIntegrator() = default;

    // Batch intersection API (CPU or GPU)
    virtual void intersect_batch(
        const std::vector<Ray>& rays,
        std::vector<HitRecord>& hits
    ) const = 0;
};
