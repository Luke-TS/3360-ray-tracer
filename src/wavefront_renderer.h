#pragma once

#include <vector>
#include <iostream>
#include <algorithm>

#include "hittable.h"
#include "ray_state.h"
#include "pixel_state.h"
#include "ray_integrator.h"   // HitInfo, RayIntegrator
#include "camera.h"
#include "scene.h"
#include "color.h"

// forward declaration
double random_double(); 

class WavefrontRenderer {
public:
    WavefrontRenderer(
        const Scene&      world,
        const camera&     cam,
        RayIntegrator&    integrator,
        int               max_depth        = 10,
        int               max_samples= 128,
        int               batch_size       = 8192
    )
        : world(world)
        , cam(cam)
        , integrator(integrator)
        , max_depth(max_depth)
        , max_ssp(max_samples)
        , batch_size(batch_size)
    {}

    // main wavefront rendering loop
    void render() {
        // adaptive sampling parameters
        const float adaptive_rel_thesh = 0.06; // per channel
        const int   minimum_samples = 16;      // prevent early termination

        const int width  = cam.get_image_width();
        const int height = cam.get_image_height();
        const int npix   = width * height;

        std::vector<PixelState> pixels(npix);
        std::vector<Color>      framebuffer(npix);

        std::vector<RayState> ray_queue;
        std::vector<RayState> next_ray_queue;
        ray_queue.reserve(batch_size);
        next_ray_queue.reserve(batch_size);

        // loop up to maximum number of samples
        for (int s = 0; s < max_ssp; ++s) {

            ray_queue.clear();

            // generate samples for non-convergent pixels
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int idx = y * width + x;
                    PixelState& ps = pixels[idx];

                    if(ps.converged) continue;

                    RayState rs;
                    rs.r           = cam.get_ray(x, y);
                    rs.pixel_index = idx;
                    rs.depth       = 0;
                    rs.throughput  = Color(1,1,1);

                    ray_queue.push_back(rs);
                }
            }
            std::clog << "Sample " << s << " ray_queue size = " << ray_queue.size() << "\n";

            // process samples
            while (!ray_queue.empty()) {
                size_t offset = 0;

                while (offset < ray_queue.size()) {
                    // batch up to batch_size rays
                    const size_t count = std::min(
                        (size_t)batch_size,
                        ray_queue.size() - offset
                    );

                    // Build batch of rays
                    std::vector<Ray> batch_rays(count);
                    for (size_t i = 0; i < count; ++i) {
                        batch_rays[i] = ray_queue[offset + i].r;
                    }

                    // Intersect batch via integrator (CPU or GPU)
                    std::vector<HitRecord> hits;
                    integrator.intersect_batch(batch_rays, hits);

                    // create local vectors for each thread
                    int thread_count = omp_get_max_threads();
                    std::vector<std::vector<RayState>> thread_local_queues(thread_count);

                    // process ray intersections in parallel
                    #pragma omp parallel for schedule(dynamic)
                    for (int i = 0; i < (int)count; i++) {
                        int tid = omp_get_thread_num();

                        RayState rs      = ray_queue[offset + i];   // Make local copy (safer)
                        PixelState& ps   = pixels[rs.pixel_index];
                        const HitRecord& rec = hits[i];

                        const Ray& r = batch_rays[i];

                        // luminance accululator
                        Color path_L = Color(0,0,0);

                        // terminate by miss or max depth
                        if (!rec.hit || rs.depth >= max_depth) {
                            path_L += rs.throughput * background(r);

                            record_sample(ps, path_L);

                            if (!ps.converged &&
                                is_converged(ps, adaptive_rel_thesh, minimum_samples))
                                ps.converged = true;

                            continue;
                        }

                        // terminate by hitting emissive material
                        Color emitted = rec.mat->emitted(rec.u, rec.v, rec.p);
                        if (!emitted.near_zero()) {
                            path_L += rs.throughput * emitted;

                            record_sample(ps, path_L);

                            if (!ps.converged &&
                                is_converged(ps, adaptive_rel_thesh, minimum_samples))
                                ps.converged = true;

                            continue;
                        }

                        // scattering
                        Ray   scattered;
                        Color attenuation;

                        // terminate if material doesn't scatter
                        if (!rec.mat->scatter(r, rec, attenuation, scattered)) {
                            record_sample(ps, path_L);

                            if (!ps.converged &&
                                is_converged(ps, adaptive_rel_thesh, minimum_samples))
                                ps.converged = true;

                            continue;
                        }

                        // create child ray for non-converged paths
                        if (ps.converged)
                            continue;

                        RayState child;
                        child.r           = scattered;
                        child.pixel_index = rs.pixel_index;
                        child.depth       = rs.depth + 1;
                        child.throughput  = rs.throughput * attenuation;

                        // russian roulette termination
                        if (child.depth > 5) {
                            double p = std::max({
                                child.throughput.x(),
                                child.throughput.y(),
                                child.throughput.z()
                            });
                            p = std::clamp(p, 0.1, 0.95);

                            if (random_double() > p) {
                                record_sample(ps, path_L);

                                if (!ps.converged &&
                                    is_converged(ps, adaptive_rel_thesh, minimum_samples))
                                    ps.converged = true;

                                continue;
                            }

                            child.throughput /= p;
                        }

                        thread_local_queues[tid].push_back(child);
                    }

                    // merge thread queues
                    for (int t = 0; t < thread_count; t++) {
                        next_ray_queue.insert(
                            next_ray_queue.end(),
                            thread_local_queues[t].begin(),
                            thread_local_queues[t].end()
                        );
                    }

                    offset += count;
                }

                // prepare queues for next batch
                ray_queue.clear();
                ray_queue.swap(next_ray_queue);
                next_ray_queue.clear();
            }
        }

        // resolve pixel states
        for (int i = 0; i < npix; i++) {
            if (pixels[i].samples > 0)
                framebuffer[i] = pixels[i].sum / (float)pixels[i].samples;
            else
                framebuffer[i] = Color(0,0,0);
        }

        // output to PPM
        std::cout << "P3\n" << width << ' ' << height << "\n255\n";

        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                Color c = framebuffer[j * width + i];
                write_color(std::cout, c);  // gamma + clamp
            }
        }
    }

private:
    const Scene&   world;
    const camera&  cam;
    RayIntegrator& integrator;
    int            max_depth;
    int            max_ssp;
    int            batch_size;

    static Color background(const Ray& r) {
        // gradient background
        Vec3 unit_direction = unit_vector(r.direction());
        auto t = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - t) * Color(1.0, 1.0, 1.0)
        + t         * Color(0.5, 0.7, 1.0);
    }
};

