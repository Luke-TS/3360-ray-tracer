#pragma once

#include "hittable.h"
#include "material.h"
#include "aabb.h"

class xy_rect : public Hittable {
public:
    xy_rect() {}

    xy_rect(double x0, double x1, double y0, double y1, double k,
            std::shared_ptr<Material> mat)
        : mat(mat), x0(x0), x1(x1), y0(y0), y1(y1), k(k) {}

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override {
        auto t = (k - r.origin().z()) / r.direction().z();
        if (!ray_t.surrounds(t))
            return false;

        auto x = r.origin().x() + t * r.direction().x();
        auto y = r.origin().y() + t * r.direction().y();

        if (x < x0 || x > x1 || y < y0 || y > y1)
            return false;

        rec.u = (x - x0) / (x1 - x0);
        rec.v = (y - y0) / (y1 - y0);
        rec.t = t;

        Vec3 outward_normal = Vec3(0, 0, 1);
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;
        rec.p = r.at(t);

        return true;
    }

    Aabb bounding_box() const override {
        // add a small thickness to prevent zero-width box
        return Aabb(Point3(x0, y0, k - 0.0001), Point3(x1, y1, k + 0.0001));
    }

    int type_id() const override { return HITTABLE_SQUARE; }
    int object_index() const override { return index; }
    void set_object_index(int i) override { index = i; }

private:
    std::shared_ptr<Material> mat;
    double x0, x1, y0, y1, k;
    int index;
};

class xz_rect : public Hittable {
public:
    xz_rect() {}

    xz_rect(double x0, double x1, double z0, double z1, double k,
            std::shared_ptr<Material> mat)
        : mat(mat), x0(x0), x1(x1), z0(z0), z1(z1), k(k) {}

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override {
        auto t = (k - r.origin().y()) / r.direction().y();
        if (!ray_t.surrounds(t))
            return false;

        auto x = r.origin().x() + t * r.direction().x();
        auto z = r.origin().z() + t * r.direction().z();
        if (x < x0 || x > x1 || z < z0 || z > z1)
            return false;

        rec.u = (x - x0) / (x1 - x0);
        rec.v = (z - z0) / (z1 - z0);
        rec.t = t;

        Vec3 outward_normal = Vec3(0, 1, 0);
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;
        rec.p = r.at(t);

        return true;
    }

    Aabb bounding_box() const override {
        return Aabb(Point3(x0, k - 0.0001, z0), Point3(x1, k + 0.0001, z1));
    }

    int type_id() const override { return HITTABLE_SQUARE; }
    int object_index() const override { return index; }
    void set_object_index(int i) override { index = i; }

private:
    std::shared_ptr<Material> mat;
    double x0, x1, z0, z1, k;
    int index;
};

class yz_rect : public Hittable {
public:
    yz_rect() {}

    yz_rect(double y0, double y1, double z0, double z1, double k,
            std::shared_ptr<Material> mat)
        : mat(mat), y0(y0), y1(y1), z0(z0), z1(z1), k(k) {}

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override {
        auto t = (k - r.origin().x()) / r.direction().x();
        if (!ray_t.surrounds(t))
            return false;

        auto y = r.origin().y() + t * r.direction().y();
        auto z = r.origin().z() + t * r.direction().z();

        if (y < y0 || y > y1 || z < z0 || z > z1)
            return false;

        rec.u = (y - y0) / (y1 - y0);
        rec.v = (z - z0) / (z1 - z0);
        rec.t = t;

        Vec3 outward_normal = Vec3(1, 0, 0);
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;
        rec.p = r.at(t);

        return true;
    }

    Aabb bounding_box() const override {
        return Aabb(Point3(k - 0.0001, y0, z0), Point3(k + 0.0001, y1, z1));
    }

    int type_id() const override { return HITTABLE_SQUARE; }
    int object_index() const override { return index; }
    void set_object_index(int i) override { index = i; }

private:
    std::shared_ptr<Material> mat;
    double y0, y1, z0, z1, k;
    int index;
};
