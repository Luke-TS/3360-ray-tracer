#pragma once

#include "scene/scene.h"
#include "scene/camera.h"

#include "integrator/sampler.h"

namespace rt::renderer {

class MegaKernel {
public:
    MegaKernel(scene::Scene& scene, scene::Camera& camera, integrator::Sampler& sampler)
        : sampler_(sampler), world_(scene), cam_(camera) {}

    void Render() {
        cam_.Initialize();
        int width = cam_.get_image_width();
        int height = cam_.get_image_height();
        long total_samples = 0;

        std::vector<core::Color> framebuffer(width * height);

        #pragma omp parallel for schedule(dynamic)
        for( int y = 0; y < height; y++ ) {
            
            int thread_id = omp_get_thread_num();

            // atomic progress logging
            #pragma omp critical
            {
                std::clog << "\rThread " << thread_id
                    << " processing scanline: " << y
                    << " (" << height - y << " remaining) "
                    << std::flush;
            }

            for( int x = 0; x < width; x++ ) {
                // aquire pixel color using sampler
                core::Color pixel_color;
                int num_samples = sampler_.SamplePixel(pixel_color, world_, cam_, x, y);
                total_samples += num_samples;

                // add color to framebuffer
                framebuffer[y * width + x] = pixel_color;
            }
        }

        std::cout << "P3\n" << width << ' ' << height << "\n255\n";
        for( auto& c: framebuffer ) {
            write_color(std::cout, c); 
        }

        std::clog << "\rDone. Total samples: " << total_samples << "Per pixel: " << total_samples / (width * height) <<"\n";
    }
private:
    integrator::Sampler& sampler_;
    scene::Scene& world_;
    scene::Camera& cam_;
};

} // namespace rt::renderer
