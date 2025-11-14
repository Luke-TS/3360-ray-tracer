#pragma once

#include "core/interval.h"

#include "aabb.h"
#include "hittable.h"
#include "mesh.h"

#include "random.h"
#include "scene/scene.h"

#include <algorithm>

namespace rt::geom {

class BvhNode : public Hittable {
  public:
    std::vector<std::shared_ptr<Hittable>> primitives;
    std::shared_ptr<Hittable> left;
    std::shared_ptr<Hittable> right;

    BvhNode(Mesh& mesh) : BvhNode(mesh.tris) {}

    BvhNode(scene::Scene& list) : BvhNode(list.objects_, 0, list.objects_.size()) {
        // There's a C++ subtlety here. This constructor (without span indices) creates an
        // implicit copy of the hittable list, which we will modify. The lifetime of the copied
        // list only extends until this constructor exits. That's OK, because we only need to
        // persist the resulting bounding volume hierarchy.
    }

    BvhNode(std::vector<std::shared_ptr<Hittable>>& objects, size_t start, size_t end) {
        auto axis = core::RandomInt(0, 2);
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
                Aabb b = obj->BoundingBox();
                temp_box = first_box ? b : Aabb(temp_box, b);
                first_box = false;
            }
            bbox_ = temp_box;
        } else {
            bbox_ = Aabb(left->BoundingBox(), right->BoundingBox());
        }
    }

    bool Hit(const core::Ray& r, core::Interval ray_t, HitRecord& rec) const override {
        if (!bbox_.hit(r, ray_t)) {
            return false;
        }

        bool hit_anything = false;
        HitRecord temp_rec;
        auto closest_so_far = ray_t.max_;

        // check for leaf node
        if (!primitives.empty()) {
            for (auto& obj : primitives) {
                if (obj->Hit(r, core::Interval(ray_t.min_, closest_so_far), temp_rec)) {
                    hit_anything = true;
                    closest_so_far = temp_rec.t;
                    rec = temp_rec;
                }
            }
            return hit_anything;
        }

        // internal node
        bool hit_left = left->Hit(r, ray_t, rec);
        bool hit_right = right->Hit(r, core::Interval(ray_t.min_, hit_left ? rec.t : ray_t.max_), rec);

        return hit_left || hit_right;
    }

    Aabb BoundingBox() const override { return bbox_; }

    // ignore: functions only used by primitives
    virtual int TypeId() const override { return -1; }
    virtual int ObjectIndex() const override { return -1; }
    virtual void set_object_index(int i) override {}

  private:
    Aabb bbox_;

    static bool box_compare(
        const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b, int axis_index
    ) {
        auto a_axis_interval = a->BoundingBox().axis_interval(axis_index);
        auto b_axis_interval = b->BoundingBox().axis_interval(axis_index);
        return a_axis_interval.min_ < b_axis_interval.min_;
    }

    static bool box_x_compare (const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b) {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare (const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b) {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare (const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b) {
        return box_compare(a, b, 2);
    }
};

} // namespace rt::geom
