#pragma once

#include <cmath>
#include <omp.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <sched.h>
#include <unordered_map>

#include "color.h"
#include "hittable.h"
#include "main.h"
#include "material.h"
#include "vec3.h"

using json = nlohmann::json;

struct CameraConfig {
    double aspect_ratio = 16/9.0;
    int    image_width  = 400;
    int    samples_per_pixel = 50;
    int    max_depth    = 10;

    double vfov = 90.0;
    vec3   lookfrom = point3(0,0,0);
    vec3   lookat   = point3(0,0,-1);
    vec3   vup      = point3(0,1,0);

    double defocus_angle = 0.0;
    double focus_dist    = 10.0;
};

inline CameraConfig parseCamera(const json& j) {
    CameraConfig cfg;
    cfg.aspect_ratio = j.value("aspectRatio", cfg.aspect_ratio);
    cfg.image_width = j.value("imageWidth", cfg.image_width);
    cfg.samples_per_pixel = j.value("samplesPerPixel", cfg.samples_per_pixel);
    cfg.max_depth = j.value("maxDepth", cfg.max_depth);

    cfg.vfov = j.value("vfov", cfg.vfov);
    cfg.lookfrom = { j["lookfrom"][0], j["lookfrom"][1], j["lookfrom"][2] };
    cfg.lookat   = { j["lookat"][0],   j["lookat"][1],   j["lookat"][2]   };
    cfg.vup      = { j["vup"][0],      j["vup"][1],      j["vup"][2]      };

    cfg.defocus_angle = j.value("defocusAngle", cfg.defocus_angle);
    cfg.focus_dist = j.value("focusDist", cfg.focus_dist);

    return cfg;
}

inline std::unordered_map<std::string, CameraConfig> loadCameras(const std::string& filename) {
    std::ifstream f(filename);
    json data = json::parse(f);

    std::unordered_map<std::string, CameraConfig> cameras;
    for (auto& [name, cam] : data.items()) {
        cameras[name] = parseCamera(cam);
    }
    return cameras;
}

class camera {
public:
    double aspect_ratio = 1.0;
    int    image_width  = 100;
    int    max_depth = 10;
    int    samples_per_pixel = 10;

    double vfov = 90.0;
    vec3   lookfrom = point3(0,0,0); // position of camera
    vec3   lookat   = point3(0,0,-1); // direction of viewport/scene
    vec3   vup      = point3(0,1,0); // vector pointing up from camera
    
    double defocus_angle = 0;
    double focus_dist = 10;

    void set_from_config(const CameraConfig& cfg) {
        aspect_ratio = cfg.aspect_ratio;
        image_width = cfg.image_width;
        max_depth = cfg.max_depth;
        samples_per_pixel = cfg.samples_per_pixel;

        vfov = cfg.vfov;
        lookfrom = cfg.lookfrom;
        lookat = cfg.lookat;
        vup = cfg.vup;

        defocus_angle = cfg.defocus_angle;
        focus_dist = cfg.focus_dist;
    }

    // initializes private variables from public config
    void initialize() {
        // calculate correct image height
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height; // height at least 1
        
        center = lookfrom;

        // camera
        double theta = degrees_to_radians(vfov);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focus_dist;
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

        auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    // get random ray at index (i, j)
    virtual ray get_ray(int i, int j) const {
        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                            + ((i + offset.x()) * pixel_delta_u)
                            + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }


    // used to aquire pixel value
    virtual color get_pixel(const ray& r, int depth, const Hittable& world) const {
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
            color emitted = rec.mat->emitted(rec.u, rec.v, rec.p);

            if( rec.mat->scatter(r, rec, attenuation, scattered) ) {
                return emitted + attenuation * get_pixel(scattered, depth-1, world);
            }
            else {
                return emitted;
            }
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0); // normalize y to 0.0 <= z <= 1.0
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0); // white to blue gradient
    }

    int get_image_height() {
        return image_height;
    }

    int get_image_width() {
        return image_width;
    }

private:
    int    image_height;
    double pixel_samples_scale;
    point3 center;
    point3 pixel00_loc;
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    vec3 u, v, w; // camera basis vectors
    vec3 defocus_disk_u;
    vec3 defocus_disk_v;


    point3 defocus_disk_sample() const {
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    vec3 sample_square() const {
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

};

class color_camera : public camera {

};

// get_pixel returns red depth map in red gradient
// rays are normalized to 1 unit
// 0 units = 100% red
// max_dist units = black
class depth_camera : public camera {
public:
    virtual color get_pixel(const ray& r, int depth, const Hittable& world) const override {
        if( depth <= 0 ) {
            return color(0,0,0);
        }

        hit_record rec;
        ray r_norm = ray(r.origin(), unit_vector(r.direction()));


        // shadow acne is caused by rounding errors resulting in intersection
        // points slightly inside or outside surfaces
        // use interval with t >= 0.001 to avoid repeat intersections
        if(world.hit(r_norm, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            return color( rec.t < max_dist ? rec.t : 0,0,0);
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0); // normalize y to 0.0 <= z <= 1.0
        return color(0, 0, 0); // return black
    }
private:
    int max_dist = 20;
};
