#pragma once

#include "hittable.h"
#include "interval.h"
#include "main.h"
#include "vec3.h"
#include <memory>

class sphere : public Hittable {
public:
    sphere(const point3& center, double radius, std::shared_ptr<material> mat) 
            : center(center), radius(std::fmax(0,radius)), mat(mat) {
        vec3 radius_vec = vec3(radius, radius, radius);
        bbox = aabb(point3(center + radius_vec), point3(center - radius_vec));
    }

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        //g_num_primitive_tests++;
        vec3 oc = center - r.origin();
        auto a = r.direction().length_squared();
        auto h = dot(r.direction(), oc);
        auto c = oc.length_squared() - radius*radius;

        auto discriminant = h*h - a*c;
        if( discriminant < 0 ) {
            return false;
        }

        auto sqrtd = std::sqrt(discriminant);

        // find nearest root in range
        auto root = (h - sqrtd) / a;
        if(!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if(!ray_t.surrounds(root)) {
                return false;
            }
        }

        // update hit record reference
        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        rec.mat = mat;

        return true;
    }

    virtual aabb bounding_box() const override {
        return bbox;
    }

    static void get_sphere_uv(const point3& p, double& u, double& v) {
        auto theta = std::acos(-p.y());
        auto phi = std::atan2(-p.z(), p.x()) + pi;

        u = phi / (2*pi);
        v = theta / pi;
    }

private:
    point3 center;
    double radius;
    std::shared_ptr<material> mat;
    aabb bbox;
};

