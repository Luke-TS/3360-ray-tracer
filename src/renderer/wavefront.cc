#include "renderer/wavefront.h"

#include "material/material.h"
#include "math_utils.h"
#include "scene/scene.h"
#include "scene/camera.h"
#include "geom/hittable.h"
#include "integrator/ray_integrator.h"
#include <omp.h>

using namespace rt;

namespace rt::renderer {

// constructor
WavefrontRenderer::WavefrontRenderer(
    const scene::Scene& world,
    const scene::Camera& cam,
    integrator::RayIntegrator& integrator,
    int max_depth,
    int max_samples,
    int batch_size
)
    : world(world)
    , cam(cam)
    , integrator(integrator)
    , max_depth(max_depth)
    , max_ssp(max_samples)
    , batch_size(batch_size)
{}

// background helper
core::Color WavefrontRenderer::background(const core::Ray& r) {
    core::Vec3 unit_direction = core::Normalize(r.direction());
    auto t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * core::Color(1.0, 1.0, 1.0)
         + t         * core::Color(0.5, 0.7, 1.0);
}

void WavefrontRenderer::Render() {

    const float kRelThresh  = 0.05;  // Adaptive threshold
    const int   kMinSamples = 16;

    const int width  = cam.get_image_width();
    const int height = cam.get_image_height();
    const int npix   = width * height;

    std::vector<integrator::PixelState> pixels(npix);
    std::vector<core::Color> framebuffer(npix);

    std::vector<integrator::RayState> ray_queue;
    std::vector<integrator::RayState> next_ray_queue;
    ray_queue.reserve(batch_size);
    next_ray_queue.reserve(batch_size);

    for (int s = 0; s < max_ssp; ++s) {

        ray_queue.clear();

        // Generate primary rays for non-converged pixels
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {

                int idx = y * width + x;
                auto& ps = pixels[idx];

                if (ps.converged)
                    continue;

                integrator::RayState rs;
                rs.r           = cam.GetRay(x, y);
                rs.pixel_index = idx;
                rs.depth       = 0;
                rs.throughput  = core::Color(1,1,1);

                ray_queue.push_back(rs);
            }
        }

        std::clog << "Sample " << s
            << "    queue=" << ray_queue.size() << "\n";

        // Process queue
        while (!ray_queue.empty()) {

            size_t offset = 0;

            while (offset < ray_queue.size()) {

                size_t count = std::min(
                    (size_t)batch_size,
                    ray_queue.size() - offset
                );

                // Build batch
                std::vector<core::Ray> batch_rays(count);
                for (size_t i = 0; i < count; ++i)
                    batch_rays[i] = ray_queue[offset + i].r;

                // Intersect
                std::vector<geom::HitRecord> hits;
                integrator.IntersectBatch(batch_rays, hits);

                int thread_count = omp_get_max_threads();
                std::vector<std::vector<integrator::RayState>>
                local_queues(thread_count);

                #pragma omp parallel for schedule(dynamic)
                for (int i = 0; i < (int)count; i++) {

                    int tid = omp_get_thread_num();
                    auto rs = ray_queue[offset + i];

                    auto& ps       = pixels[rs.pixel_index];
                    const auto& rec = hits[i];
                    const auto& r   = batch_rays[i];

                    core::Color L = core::Color(0,0,0);

                    // Miss or depth limit
                    if (!rec.hit || rs.depth >= max_depth) {
                        L += rs.throughput * background(r);
                        integrator::RecordSample(ps, L);
                        if (!ps.converged &&
                            integrator::IsConverged(ps, kRelThresh, kMinSamples))
                            ps.converged = true;
                        continue;
                    }

                    // Hit emissive
                    core::Color emitted =
                        rec.mat->Emitted(rec.u, rec.v, rec.p);

                    if (!emitted.NearZero()) {
                        L += rs.throughput * emitted;
                        integrator::RecordSample(ps, L);
                        if (!ps.converged &&
                            integrator::IsConverged(ps, kRelThresh, kMinSamples))
                            ps.converged = true;
                        continue;
                    }

                    // BSDF reflections
                    core::Vec3 wo = -core::Normalize(r.direction());
                    core::Vec3 wi;
                    float pdf = 0.0f;
                    core::Color f;

                    if (!rec.mat->Sample(rec, wo, wi, pdf, f)) {
                        // no scattering
                        integrator::RecordSample(ps, L);
                        if (!ps.converged &&
                            integrator::IsConverged(ps, kRelThresh, kMinSamples))
                            ps.converged = true;
                        continue;
                    }

                    if (ps.converged)
                        continue;

                    integrator::RayState child;
                    child.r           = core::Ray(rec.p, wi);
                    child.pixel_index = rs.pixel_index;
                    child.depth       = rs.depth + 1;

                    if (rec.mat->IsSpecular()) {
                        // delta BSDF: f already encodes the contribution
                        // DON'T apply cosθ or divide by pdf
                        child.throughput = rs.throughput * f;
                    } else {
                        if (pdf < 1e-6f) {
                            integrator::RecordSample(ps, L);
                            if (!ps.converged &&
                                integrator::IsConverged(ps, kRelThresh, kMinSamples))
                                ps.converged = true;
                            continue;
                        }

                        float cos_theta = std::max(
                            0.0f,
                            static_cast<float>(core::Dot(wi, rec.normal))
                        );

                        child.throughput = rs.throughput * f * cos_theta / pdf;
                    }

                    // Russian roulette for path termination
                    if (child.depth > 5) {
                        double p = std::max({
                            child.throughput.x(),
                            child.throughput.y(),
                            child.throughput.z()
                        });
                        p = std::clamp(p, 0.1, 0.95);

                        if (core::RandomDouble() > p) {
                            integrator::RecordSample(ps, L);
                            if (!ps.converged &&
                                integrator::IsConverged(ps, kRelThresh, kMinSamples))
                                ps.converged = true;
                            continue;
                        }
                        child.throughput /= p;
                    }

                    local_queues[tid].push_back(child);
                }

                // Merge queues
                for (int t = 0; t < thread_count; t++) {
                    next_ray_queue.insert(
                        next_ray_queue.end(),
                        local_queues[t].begin(),
                        local_queues[t].end()
                    );
                }

                offset += count;
            }

            ray_queue.clear();
            ray_queue.swap(next_ray_queue);
            next_ray_queue.clear();
        }
    }

    // Write framebuffer
    for (int i = 0; i < npix; i++) {
        if (pixels[i].samples > 0)
            framebuffer[i] =
                pixels[i].sum / (float)pixels[i].samples;
        else
            framebuffer[i] = core::Color(0,0,0);
    }

    // Output PPM
    std::cout << "P3\n" << width << ' ' << height << "\n255\n";
    for (int j = 0; j < height; ++j)
        for (int i = 0; i < width; ++i)
            write_color(std::cout, framebuffer[j * width + i]);
}

} // namespace rt::renderer
