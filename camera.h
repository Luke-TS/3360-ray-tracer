#pragma once

#include <cmath>
#include <iostream>

#include "color.h"
#include "hittable.h"
#include "main.h"
#include "material.h"
#include "vec3.h"

class camera {
public:
    double aspect_ratio = 1.0;
    int    image_width  = 100;
    int    samples_per_pixel = 10;
    int    max_depth = 10;
    vec3   lookfrom = point3(0,0,0); // position of camera
    vec3   lookat   = point3(0,0,-1); // direction of viewport/scene
    vec3   vup      = point3(0,1,0); // vector pointing up from camera

    double vfov = 120.0;

    void render(const hittable& world) {
        initialize();
        
        // image metadata
        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for( int j = 0; j < image_height; j++ ) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for( int i = 0; i < image_width; i++ ) {
                auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
                auto ray_direction = pixel_center - center;
                ray r(center, ray_direction);

                color pixel_color(0,0,0);
                for(int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                write_color(std::cout, pixel_samples_scale * pixel_color);
            }
        }

        std::clog << "\rDone.                 \n";
    }

private:
    int image_height;
    double pixel_samples_scale;
    point3 center;
    point3 pixel00_loc;
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    vec3 u, v, w; // camera basis vectors

    void initialize() {

        // calculate correct image height
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height; // height at least 1
        
        pixel_samples_scale = 1.0 / samples_per_pixel;
        
        center = lookfrom;

        // camera
        auto focal_length = (lookfrom - lookat).length(); // distance from camera to center of viewport
        double theta = degrees_to_radians(vfov);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focal_length;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        w = unit_vector(lookfrom - lookat); // back orthogonal to viewport
        u = unit_vector(cross(vup, w)); // right along viewport
        v = cross(w, u); // up along viewport

        // vectors across horizontal and vertial viewport edges originating in top left of viewport
        vec3 viewport_u = viewport_width * u;
        vec3 viewport_v = viewport_height * -v;

        // horizontal and vertical delta vectors from pixel to pixel
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = center - (focal_length * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    ray get_ray(int i, int j) const {
        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                            + ((i + offset.x()) * pixel_delta_u)
                            + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    vec3 sample_square() const {
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        if( depth <= 0 ) {
            return color(0,0,0);
        }

        hit_record rec;

        // shadow acne is caused by rounding errors resulting in intersection
        // points slightly inside or outside surfaces
        // use interval with t >= 0.001 to avoid repeat intersections
        if(world.hit(r, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if( rec.mat->scatter(r, rec, attenuation, scattered) ) {
                return attenuation * ray_color(scattered, depth-1, world);
            }
            return color(0,0,0);
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0); // normalize y to 0.0 <= z <= 1.0
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0); // white to blue gradient
    }
};
