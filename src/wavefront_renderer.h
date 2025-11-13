#pragma once

#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "hittable.h"
#include "ray_state.h"
#include "pixel_state.h"
#include "ray_integrator.h"   // HitInfo, RayIntegrator
#include "camera.h"
#include "scene.h"
#include "material.h"
#include "color.h"

// Forward declaration of random_double if it's in a utility header
double random_double();  // adapt to your existing RNG declaration

class WavefrontRenderer {
public:
    WavefrontRenderer(
        const Scene&      world,
        const camera&     cam,
        RayIntegrator&    integrator,
        int               max_depth        = 10,
        int               samples_per_pixel= 16,
        int               batch_size       = 8192
    )
        : world(world)
        , cam(cam)
        , integrator(integrator)
        , max_depth(max_depth)
        , spp(samples_per_pixel)
        , batch_size(batch_size)
    {}

    // Render to std::cout as PPM (like your old renderer)
    void render() {
        const int width  = cam.get_image_width();
        const int height = cam.get_image_height();
        const int npix   = width * height;

        std::vector<PixelState> pixels(npix);
        std::vector<color>      framebuffer(npix);

        std::vector<RayState> ray_queue;
        std::vector<RayState> next_ray_queue;
        ray_queue.reserve(batch_size);
        next_ray_queue.reserve(batch_size);

        // Outer loop: samples-per-pixel (anti-aliasing)
        for (int s = 0; s < spp; ++s) {

            // 1. Generate primary rays for all pixels for this sample
            ray_queue.clear();

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int idx = y * width + x;

                    // get random ray from camera
                    ray r = cam.get_ray(x, y);

                    RayState rs;
                    rs.r           = r;
                    rs.pixel_index = idx;
                    rs.depth       = 0;
                    rs.throughput  = color(1,1,1);

                    ray_queue.push_back(rs);
                }
            }

            // 2. Wavefront loop — process all bounces for this sample
            while (!ray_queue.empty()) {
                size_t offset = 0;

                while (offset < ray_queue.size()) {
                    // batch up to batch_size rays
                    const size_t count = std::min(
                        (size_t)batch_size,
                        ray_queue.size() - offset
                    );

                    // Build batch of rays
                    std::vector<ray> batch_rays(count);
                    for (size_t i = 0; i < count; ++i) {
                        batch_rays[i] = ray_queue[offset + i].r;
                    }

                    // Intersect batch via integrator (CPU or GPU)
                    std::vector<hit_record> hits;
                    integrator.intersect_batch(batch_rays, hits);

                    // Process results on CPU

                    // temporary: one thread-local next-ray queue per thread
                    int thread_count = omp_get_max_threads();
                    std::vector<std::vector<RayState>> thread_local_queues(thread_count);


                    #pragma omp parallel for schedule(dynamic)
                    for (int i = 0; i < (int)count; i++) {
                        int tid = omp_get_thread_num();

                        RayState& rs      = ray_queue[offset + i];
                        PixelState& ps    = pixels[rs.pixel_index];
                        const hit_record& rec = hits[i];

                        const ray& r = batch_rays[i];

                        // MISS or max depth → accumulate *background*
                        if (!rec.hit || rs.depth >= max_depth) {
                            color L = rs.throughput * background(r);
                            ps.sum += L;
                            ps.samples++;
                            continue;
                        }

                        // Emitters ALWAYS contribute (just like in get_pixel)
                        color emitted = rec.mat->emitted(rec.u, rec.v, rec.p);
                        color L = rs.throughput * emitted;
                        ps.sum += L;
                        ps.samples++;

                        // Now scatter
                        ray   scattered;
                        color attenuation;

                        if (!rec.mat->scatter(r, rec, attenuation, scattered)) {
                            // Non-scattering material = only emitted (same as recursive)
                            continue;
                        }

                        // Spawn child ray
                        RayState child;
                        child.r           = scattered;
                        child.pixel_index = rs.pixel_index;
                        child.depth       = rs.depth + 1;
                        child.throughput  = rs.throughput * attenuation;

                        // Optional Russian roulette
                        /*
                        if (child.depth > 5) {
                            double p = std::max({
                                child.throughput.x(),
                                child.throughput.y(),
                                child.throughput.z()
                            });
                            p = std::clamp(p, 0.1, 0.95);
                            if (random_double() > p) continue;
                            child.throughput /= p;
                        }
                        */

                        thread_local_queues[tid].push_back(child);
                    }

                    // merge all thread-local queues into next_ray_queue
                    for (int t = 0; t < thread_count; t++) {
                        next_ray_queue.insert(
                            next_ray_queue.end(),
                            thread_local_queues[t].begin(),
                            thread_local_queues[t].end()
                        );
                    }

                    offset += count;
                }

                ray_queue.clear();
                ray_queue.swap(next_ray_queue);
                next_ray_queue.clear();
            }

            std::clog << "\rSample " << (s+1) << "/" << spp << std::flush;
        }


        // 3. Resolve pixel states into framebuffer (average, no gamma yet)
        for (int i = 0; i < npix; i++) {
            if (pixels[i].samples > 0)
                framebuffer[i] = pixels[i].sum / (float)pixels[i].samples;
            else
                framebuffer[i] = color(0,0,0);
        }

        // 4. Output PPM
        std::cout << "P3\n" << width << ' ' << height << "\n255\n";

        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                color c = framebuffer[j * width + i];
                write_color(std::cout, c);  // gamma + clamp
            }
        }
    }

private:
    const Scene&   world;
    const camera&  cam;
    RayIntegrator& integrator;
    int            max_depth;
    int            spp;
    int            batch_size;

    static color background(const ray& r) {
        // Simple RTIOW-style gradient — replace with your own
        vec3 unit_direction = unit_vector(r.direction());
        auto t = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - t) * color(1.0, 1.0, 1.0)
             + t         * color(0.5, 0.7, 1.0);
    }
};

