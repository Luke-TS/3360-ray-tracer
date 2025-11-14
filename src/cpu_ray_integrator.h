#pragma once

#include "hittable.h"
#include "ray_integrator.h"
#include "scene.h"      // hit_record, Scene::hit()
#include <omp.h>

class CPURayIntegrator : public RayIntegrator {
public:
    CPURayIntegrator(const Scene* world)
        : world(world) {}

    void intersect_batch( const std::vector<Ray>& rays, std::vector<HitRecord>& hits ) const override {
        hits.resize(rays.size());

        float t_min = 0.001f;
        float t_max = std::numeric_limits<float>::infinity();

        #pragma omp parallel for
        for (size_t i = 0; i < rays.size(); ++i) {
            HitRecord rec; // your existing type
            bool ok = world->hit(rays[i], Interval(t_min, t_max), rec);

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
    const Scene* world;
};
