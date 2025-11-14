#pragma once

#include "aabb.h"
#include "hittable.h"
#include "scene.h"
#include "triangle_mesh.h"

#include <algorithm>

class BvhNode : public Hittable {
  public:
    std::vector<std::shared_ptr<Hittable>> primitives;
    shared_ptr<Hittable> left;
    shared_ptr<Hittable> right;

    BvhNode(TriangleMesh& mesh) : BvhNode(mesh.tris) {}

    BvhNode(Scene& list) : BvhNode(list.objects, 0, list.objects.size()) {
        // There's a C++ subtlety here. This constructor (without span indices) creates an
        // implicit copy of the hittable list, which we will modify. The lifetime of the copied
        // list only extends until this constructor exits. That's OK, because we only need to
        // persist the resulting bounding volume hierarchy.
    }

    BvhNode(std::vector<shared_ptr<Hittable>>& objects, size_t start, size_t end) {
        auto axis = random_int(0, 2);
        auto comparator = (axis == 0) ? box_x_compare
            : (axis == 1) ? box_y_compare
            : box_z_compare;

        size_t object_span = end - start;

        if (object_span <= 4) {
            // leaf node - just store the primitives
            for (size_t i = start; i < end; i++) {
                primitives.push_back(objects[i]);
            }
            left = nullptr;
            right = nullptr;
        } else {
            // sort by comparator and split
            std::sort(objects.begin() + start, objects.begin() + end, comparator);

            auto mid = start + object_span / 2;
            left  = std::make_shared<BvhNode>(objects, start, mid);
            right = std::make_shared<BvhNode>(objects, mid, end);
        }

        // compute bounding box
        if (!primitives.empty()) {
            // leaf: merge all prim AABBs
            Aabb temp_box;
            bool first_box = true;
            for (auto& obj : primitives) {
                Aabb b = obj->bounding_box();
                temp_box = first_box ? b : Aabb(temp_box, b);
                first_box = false;
            }
            bbox = temp_box;
        } else {
            bbox = Aabb(left->bounding_box(), right->bounding_box());
        }
    }

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override {
        if (!bbox.hit(r, ray_t)) {
            return false;
        }

        bool hit_anything = false;
        HitRecord temp_rec;
        auto closest_so_far = ray_t.max;

        // check for leaf node
        if (!primitives.empty()) {
            for (auto& obj : primitives) {
                if (obj->hit(r, Interval(ray_t.min, closest_so_far), temp_rec)) {
                    hit_anything = true;
                    closest_so_far = temp_rec.t;
                    rec = temp_rec;
                }
            }
            return hit_anything;
        }

        // internal node
        bool hit_left = left->hit(r, ray_t, rec);
        bool hit_right = right->hit(r, Interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

        return hit_left || hit_right;
    }

    Aabb bounding_box() const override { return bbox; }

    // ignore: functions only used by primitives
    virtual int type_id() const override { return -1; }
    virtual int object_index() const override { return -1; }
    virtual void set_object_index(int i) override {}

  private:
    Aabb bbox;

    static bool box_compare(
        const shared_ptr<Hittable> a, const shared_ptr<Hittable> b, int axis_index
    ) {
        auto a_axis_interval = a->bounding_box().axis_interval(axis_index);
        auto b_axis_interval = b->bounding_box().axis_interval(axis_index);
        return a_axis_interval.min < b_axis_interval.min;
    }

    static bool box_x_compare (const shared_ptr<Hittable> a, const shared_ptr<Hittable> b) {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare (const shared_ptr<Hittable> a, const shared_ptr<Hittable> b) {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare (const shared_ptr<Hittable> a, const shared_ptr<Hittable> b) {
        return box_compare(a, b, 2);
    }
};
