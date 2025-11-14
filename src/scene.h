#pragma once

#include <vector>

#include "hittable.h"

using std::make_shared;
using std::shared_ptr;

class Scene : public Hittable {
public:
    std::vector<shared_ptr<Hittable>> objects;

    Scene() {}
    Scene(shared_ptr<Hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<Hittable> object) {
        objects.push_back(object);
        bbox = Aabb(bbox, object->bounding_box()); // expand box to accomodate object
    }

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override {
        HitRecord temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for( const auto& object : objects ) {
            if (object->hit(r, Interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    Aabb bounding_box() const override { return bbox; }

    // ignore: functions only used by primitives
    virtual int type_id() const override { return -1; }
    virtual int object_index() const override { return -1; }
    virtual void set_object_index(int i) override {}
private:
    Aabb bbox; // bounding box containing all objects
};
