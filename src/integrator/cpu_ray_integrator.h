#pragma once

#include "ray_integrator.h"

#include "geom/hittable.h"

#include "scene/scene.h"      

#include <omp.h>

namespace rt::integrator {

class CPURayIntegrator : public RayIntegrator {
public:
    CPURayIntegrator(const scene::Scene* world)
        : world_(world) {}

    void IntersectBatch( const std::vector<core::Ray>& rays, std::vector<geom::HitRecord>& hits ) const override {
        hits.resize(rays.size());

        float t_min = 0.001f;
        float t_max = std::numeric_limits<float>::infinity();

        #pragma omp parallel for
        for (size_t i = 0; i < rays.size(); ++i) {
            geom::HitRecord rec; // your existing type
            bool ok = world_->Hit(rays[i], core::Interval(t_min, t_max), rec);

            rec.hit = ok;

            /*
            if (ok) {
                hi.t      = rec.t;
                hi.p      = rec.p;
                hi.normal = rec.normal;
                hi.mat    = rec.mat.get();

                hi.u          = rec.u;
                hi.v          = rec.v;
                hi.front_face = rec.front_face;
            }
            */

            hits[i] = rec;
        }
    }

private:
    const scene::Scene* world_;
};

} // namespace rt::integrator
