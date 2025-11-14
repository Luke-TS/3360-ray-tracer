#pragma once

#include "hittable.h"
#include "scene.h"
#include "triangle.h"

#include <memory>
#include <vector>
#include <array>

class TriangleMesh : public Hittable {
public:
    Scene tris;

    TriangleMesh(const std::vector<Point3>& vertices,
                  const std::vector<std::array<int,3>>& indices,
                  std::shared_ptr<Material> mat) {
        for( const auto& face : indices ) {
            auto tri = std::make_shared<Triangle>(
                vertices[face[0]],
                vertices[face[1]],
                vertices[face[2]],
                mat
            );
            tris.add(tri);
        }
    }

    virtual bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override {
        return tris.hit(r, ray_t, rec);
    }

    virtual Aabb bounding_box() const override {
        return tris.bounding_box();
    }
};
