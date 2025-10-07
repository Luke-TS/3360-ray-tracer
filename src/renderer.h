#pragma once

#include "Sampler.h"
#include "camera.h"

class Renderer {
public:
    Renderer(Scene& scene, camera& camera, Sampler& sampler)
        : world(scene), cam(camera), sampler(sampler) {}

    void render() {
        cam.initialize();
        int width = cam.get_image_width();
        int height = cam.get_image_height();

        std::vector<color> framebuffer(width * height);

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
                color pixel_color = sampler.sample_pixel(world, cam, x, y);

                // add color to framebuffer
                framebuffer[y * width + x] = pixel_color;
            }
        }

        std::cout << "P3\n" << width << ' ' << height << "\n255\n";
        for( auto& c: framebuffer ) {
            write_color(std::cout, c); 
        }

        std::clog << "\rDone.                 \n";
    }
private:
    Sampler& sampler;
    Scene& world;
    camera& cam;
};
