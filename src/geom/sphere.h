#pragma once

#include "constants.h"
#include "core/interval.h"
#include "core/vec3.h"

#include "hittable.h"

#include <memory>

namespace rt::geom {

class Sphere : public Hittable {
public:
    int gpu_index = -1;

    Sphere(const core::Point3& center, double radius, std::shared_ptr<material::Material> mat) 
            : center_(center), radius_(std::fmax(0,radius)), mat_(mat) {
        core::Vec3 radius_vec = core::Vec3(radius, radius, radius);
        bbox_ = Aabb(core::Point3(center + radius_vec), core::Point3(center - radius_vec));
    }

    virtual bool Hit(const core::Ray& r, core::Interval ray_t, HitRecord& rec) const override {
        //g_num_primitive_tests++;
        core::Vec3 oc = center_ - r.origin();
        auto a = r.direction().length_squared();
        auto h = Dot(r.direction(), oc);
        auto c = oc.length_squared() - radius_*radius_;

        auto discriminant = h*h - a*c;
        if( discriminant < 0 ) {
            return false;
        }

        auto sqrtd = std::sqrt(discriminant);

        // find nearest root in range
        auto root = (h - sqrtd) / a;
        if(!ray_t.Surrounds(root)) {
            root = (h + sqrtd) / a;
            if(!ray_t.Surrounds(root)) {
                return false;
            }
        }

        // update hit record reference
        rec.t = root;
        rec.p = r.at(rec.t);
        core::Vec3 outward_normal = (rec.p - center_) / radius_;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        rec.mat = mat_;

        return true;
    }

    virtual Aabb BoundingBox() const override {
        return bbox_;
    }

    virtual int TypeId() const override {
        return HITTABLE_SPHERE;
    }

    virtual int ObjectIndex() const override {
        return gpu_index;
    }

    virtual void set_object_index(int i) override {
        gpu_index = i;
    }

    static void get_sphere_uv(const core::Point3& p, double& u, double& v) {
        auto theta = std::acos(-p.y());
        auto phi = std::atan2(-p.z(), p.x()) + core::kPi;

        u = phi / (2 * core::kPi);
        v = theta / core::kPi;
    }

private:
    core::Point3 center_;
    double radius_;
    std::shared_ptr<material::Material> mat_;
    Aabb bbox_;
};

} // namespace rt::geom
