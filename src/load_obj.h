#pragma once
#include "triangle_mesh.h"
#include "material.h"

#include <tiny_obj_loader.h>
#include <string>
#include <stdexcept>
#include <memory>

inline std::shared_ptr<triangle_mesh> load_obj(const std::string& filename,
                                               std::shared_ptr<material> mat,
                                               float scale)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str()))
        throw std::runtime_error(warn + err);

    std::vector<point3> vertices;
    for (size_t v=0; v<attrib.vertices.size()/3; ++v)
        vertices.emplace_back(attrib.vertices[3*v+0],
                              attrib.vertices[3*v+1],
                              attrib.vertices[3*v+2]);

    std::vector<std::array<int,3>> indices;
    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f=0; f<shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) { index_offset += fv; continue; }
            indices.push_back({
                shape.mesh.indices[index_offset+0].vertex_index,
                shape.mesh.indices[index_offset+1].vertex_index,
                shape.mesh.indices[index_offset+2].vertex_index
            });
            index_offset += 3;
        }
    }

    // center the mesh
    vec3 centroid(0.0f, 0.0f, 0.0f);
    for( auto& v : vertices )
        centroid += v;
    centroid /= vertices.size();

    for( auto& v : vertices ) {
        v = v - centroid;
        v = v * scale;
    }

    return std::make_shared<triangle_mesh>(vertices, indices, mat);
}

