#pragma once

#include <cmath>
#include <omp.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <sched.h>
#include <unordered_map>

#include "core/color.h"
#include "core/constants.h"
#include "core/math_utils.h"
#include "core/interval.h"
#include "core/vec3.h"

#include "geom/hittable.h"

#include "material/material.h"

namespace rt::scene {

using json = nlohmann::json;

struct CameraConfig {
    double aspect_ratio = 16/9.0;
    int    image_width  = 400;
    int    samples_per_pixel = 50;
    int    max_depth    = 10;

    double vfov = 90.0;
    core::Vec3   lookfrom = core::Point3(0,0,0);
    core::Vec3   lookat   = core::Point3(0,0,-1);
    core::Vec3   vup      = core::Point3(0,1,0);

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

class Camera {
public:
    double aspect_ratio_ = 1.0;
    int    image_width_  = 100;
    int    max_depth_ = 10;
    int    samples_per_pixel_ = 10;

    double vfov_ = 90.0;
    core::Vec3   lookfrom_ = core::Point3(0,0,0); // position of camera
    core::Vec3   lookat   = core::Point3(0,0,-1); // direction of viewport/scene
    core::Vec3   vup_      = core::Point3(0,1,0); // vector pointing up from camera
    
    double defocus_angle_ = 0;
    double focus_dist_ = 10;

    void SetFromConfig(const CameraConfig& cfg) {
        aspect_ratio_ = cfg.aspect_ratio;
        image_width_ = cfg.image_width;
        max_depth_ = cfg.max_depth;
        samples_per_pixel_ = cfg.samples_per_pixel;

        vfov_ = cfg.vfov;
        lookfrom_ = cfg.lookfrom;
        lookat = cfg.lookat;
        vup_ = cfg.vup;

        defocus_angle_ = cfg.defocus_angle;
        focus_dist_ = cfg.focus_dist;
    }

    // initializes private variables from public config
    void Initialize() {
        // calculate correct image height
        image_height_ = int(image_width_ / aspect_ratio_);
        image_height_ = (image_height_ < 1) ? 1 : image_height_; // height at least 1
        
        center_ = lookfrom_;

        // camera
        double theta = core::DegreesToRadians(vfov_);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focus_dist_;
        auto viewport_width = viewport_height * (double(image_width_)/image_height_);

        w_ = core::Normalize(lookfrom_ - lookat); // back orthogonal to viewport
        u_ = core::Normalize(core::Cross(vup_, w_)); // right along viewport
        v_ = core::Cross(w_, u_); // up along viewport

        // vectors across horizontal and vertial viewport edges originating in top left of viewport
        core::Vec3 viewport_u = viewport_width * u_;
        core::Vec3 viewport_v = viewport_height * -v_;

        // horizontal and vertical delta vectors from pixel to pixel
        pixel_delta_u_ = viewport_u / image_width_;
        pixel_delta_v_ = viewport_v / image_height_;

        auto viewport_upper_left = center_ - (focus_dist_ * w_) - viewport_u/2 - viewport_v/2;
        pixel00_loc_ = viewport_upper_left + 0.5 * (pixel_delta_u_ + pixel_delta_v_);

        auto defocus_radius = focus_dist_ * std::tan(core::DegreesToRadians(defocus_angle_ / 2));
        defocus_disk_u_ = u_ * defocus_radius;
        defocus_disk_v_ = v_ * defocus_radius;
    }

    // get random ray at index (i, j)
    virtual core::Ray GetRay(int i, int j) const {
        auto offset = SampleSquare();
        auto pixel_sample = pixel00_loc_
                            + ((i + offset.x()) * pixel_delta_u_)
                            + ((j + offset.y()) * pixel_delta_v_);

        auto ray_origin = (defocus_angle_ <= 0) ? center_ : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;

        return core::Ray(ray_origin, ray_direction);
    }


    // used to aquire pixel value
    virtual core::Color GetPixel(const core::Ray& r, int depth, const geom::Hittable& world) const {
        if( depth <= 0 ) {
            return core::Color(0,0,0);
        }

        geom::HitRecord rec;

        // shadow acne is caused by rounding errors resulting in intersection
        // points slightly inside or outside surfaces
        // use interval with t >= 0.001 to avoid repeat intersections
        if(world.Hit(r, core::Interval(0.001, core::kInfinity), rec)) {
            core::Ray scattered;
            core::Color attenuation;
            core::Color emitted = rec.mat->Emitted(rec.u, rec.v, rec.p);

            if( rec.mat->Scatter(r, rec, attenuation, scattered) ) {
                return emitted + attenuation * GetPixel(scattered, depth-1, world);
            }
            else {
                return emitted;
            }
        }

        core::Vec3 unit_direction = core::Normalize(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0); // normalize y to 0.0 <= z <= 1.0
        return (1.0-a)*core::Color(1.0, 1.0, 1.0) + a*core::Color(0.5, 0.7, 1.0); // white to blue gradient
    }

    int get_image_height() const {
        return image_height_;
    }

    int get_image_width() const {
        return image_width_;
    }

private:
    int    image_height_;
    double pixel_samples_scale_;
    core::Point3 center_;
    core::Point3 pixel00_loc_;
    core::Vec3 pixel_delta_u_;
    core::Vec3 pixel_delta_v_;
    core::Vec3 u_, v_, w_; // camera basis vectors
    core::Vec3 defocus_disk_u_;
    core::Vec3 defocus_disk_v_;


    core::Point3 defocus_disk_sample() const {
        auto p = core::RandomInUnitDisk();
        return center_ + (p[0] * defocus_disk_u_) + (p[1] * defocus_disk_v_);
    }

    core::Vec3 SampleSquare() const {
        return core::Vec3(core::RandomDouble() - 0.5, core::RandomDouble() - 0.5, 0);
    }

};

class ColorCamera : public Camera {

};

// get_pixel returns red depth map in red gradient
// rays are normalized to 1 unit
// 0 units = 100% red
// max_dist units = black
class DepthCamera : public Camera {
public:
    virtual core::Color GetPixel(const core::Ray& r, int depth, const geom::Hittable& world) const override {
        if( depth <= 0 ) {
            return core::Color(0,0,0);
        }

        geom::HitRecord rec;
        core::Ray r_norm = core::Ray(r.origin(), core::Normalize(r.direction()));


        // shadow acne is caused by rounding errors resulting in intersection
        // points slightly inside or outside surfaces
        // use interval with t >= 0.001 to avoid repeat intersections
        if(world.Hit(r_norm, core::Interval(0.001, core::kInfinity), rec)) {
            core::Ray scattered;
            core::Color attenuation;
            return core::Color( rec.t < max_dist_ ? rec.t : 0,0,0);
        }

        core::Vec3 unit_direction = core::Normalize(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0); // normalize y to 0.0 <= z <= 1.0
        return core::Color(0, 0, 0); // return black
    }
private:
    int max_dist_ = 20;
};

} // namespace rt::scene
