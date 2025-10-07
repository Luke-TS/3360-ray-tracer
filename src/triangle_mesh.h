#pragma once

#include "hittable.h"
#include "scene.h"
#include "triangle.h"

#include <memory>
#include <vector>
#include <array>

class triangle_mesh : public Hittable {
public:
    Scene tris;

    triangle_mesh(const std::vector<point3>& vertices,
                  const std::vector<std::array<int,3>>& indices,
                  std::shared_ptr<material> mat) {
        for( const auto& face : indices ) {
            auto tri = std::make_shared<triangle>(
                vertices[face[0]],
                vertices[face[1]],
                vertices[face[2]],
                mat
            );
            tris.add(tri);
        }
    }

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        return tris.hit(r, ray_t, rec);
    }

    virtual aabb bounding_box() const override {
        return tris.bounding_box();
    }
};
