#pragma once

#include "Sampler.h"
#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"

class Renderer {
public:
    Sampler sampler;
    hittable_list world;
    camera cam;

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
                color pixel_color = sampler->sample_pixel(world, cam, x, y);

                framebuffer[x * width + y] = pixel_color;

                auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
                auto ray_direction = pixel_center - center;
                ray r(center, ray_direction);

                color pixel_color(0,0,0);
                for(int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += get_pixel(r, max_depth, world);
                }
                framebuffer[j * image_width + i] = pixel_color;
            }
        }

        std::cout << "P3\n" << width << ' ' << height << "\n255\n";
        for( auto& c: framebuffer ) {
            write_color(std::cout, c); 
        }

        std::clog << "\rDone.                 \n";
    }
};
