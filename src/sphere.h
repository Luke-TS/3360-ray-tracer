#pragma once

#include "hittable.h"
#include "interval.h"
#include "main.h"
#include "vec3.h"
#include <memory>

class Sphere : public Hittable {
public:
    int gpu_index = -1;

    Sphere(const Point3& center, double radius, std::shared_ptr<Material> mat) 
            : center(center), radius(std::fmax(0,radius)), mat(mat) {
        Vec3 radius_vec = Vec3(radius, radius, radius);
        bbox = Aabb(Point3(center + radius_vec), Point3(center - radius_vec));
    }

    virtual bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override {
        //g_num_primitive_tests++;
        Vec3 oc = center - r.origin();
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
        Vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        rec.mat = mat;

        return true;
    }

    virtual Aabb bounding_box() const override {
        return bbox;
    }

    virtual int type_id() const override {
        return HITTABLE_SPHERE;
    }

    virtual int object_index() const override {
        return gpu_index;
    }

    virtual void set_object_index(int i) override {
        gpu_index = i;
    }

    static void get_sphere_uv(const Point3& p, double& u, double& v) {
        auto theta = std::acos(-p.y());
        auto phi = std::atan2(-p.z(), p.x()) + pi;

        u = phi / (2*pi);
        v = theta / pi;
    }

private:
    Point3 center;
    double radius;
    std::shared_ptr<Material> mat;
    Aabb bbox;
};

