#include "core/math_utils.h"
#include "core/random.h"
#include "geom/bvh.h"
#include "scene/camera.h"
#include "material/material.h"
#include "scene/scene.h"
#include "integrator/sampler.h"
#include "geom/sphere.h"
#include "geom/rect.h"
#include "gpu_utils.h"
#include "material/texture.h"
#include "core/timer.h"
#include "renderer/wavefront.h"
#include "renderer/mega_kernel.h"
#include "integrator/cpu_ray_integrator.h"

#include <iomanip>
#include <iostream>
#include <ostream>

using namespace rt;

void flatten_bvh_debug(scene::Scene& scene) {
    auto world_bvh = std::make_shared<geom::BvhNode>(scene);
    std::vector<BvhNodeGPU> flat_nodes;
    std::vector<PrimativeRef> flat_prims;

    flatten_bvh(world_bvh.get(), flat_nodes, flat_prims);

    // Debug print
    std::cout << "Flattened BVH:\n";
    for (size_t i = 0; i < flat_nodes.size(); i++) {
        const auto& n = flat_nodes[i];
        std::cout << "Node " << i << ": bbox_min(" << n.bbox_min.x() << ", " << n.bbox_min.y() << ", " << n.bbox_min.z() << ") "
            << "bbox_max(" << n.bbox_max.x() << ", " << n.bbox_max.y() << ", " << n.bbox_max.z() << ") "
            << "left_first=" << n.left_or_first << " count=" << n.count << "\n";
    }

    std::cout << "\nFlattened primitives:\n";
    for (size_t i = 0; i < flat_prims.size(); i++) {
        std::cout << "Prim " << i << ": type=" << flat_prims[i].type
            << " index=" << flat_prims[i].index << "\n";
    }
}

void cornell_box(scene::Scene& world_root) {
    scene::Scene world;

    // Materials
    auto red   = std::make_shared<material::Lambertian>(core::Color(.65, .05, .05));
    auto white = std::make_shared<material::Lambertian>(core::Color(.73, .73, .73));
    auto green = std::make_shared<material::Lambertian>(core::Color(.12, .45, .15));
    auto light = std::make_shared<material::DiffuseLight>(core::Color(15, 15, 15));  // Brighter light

    // Standard Cornell Box scaling (10×10×10 world units)
    const double S = 10.0;       // side length
    const double eps = 0.01;     // small offset to avoid z-fighting

    // --- Walls ---
    world.Add(std::make_shared<geom::yz_rect>(0, S, 0, S,  S, green));   // Right wall
    world.Add(std::make_shared<geom::yz_rect>(0, S, 0, S,  0, red));     // Left wall
    world.Add(std::make_shared<geom::xz_rect>(0, S, 0, S,  0, white));   // Floor
    world.Add(std::make_shared<geom::xz_rect>(0, S, 0, S,  S, white));   // Ceiling
    world.Add(std::make_shared<geom::xy_rect>(0, S, 0, S,  S, white));   // Back wall

    // --- Light in ceiling ---
    // Standard proportion: light is ~1/3 width of box
    const double L0 = 3.0;
    const double L1 = 7.0;
    world.Add(std::make_shared<geom::xz_rect>(L0, L1, L0, L1, S - eps, light));  

    // --- Objects: Use spheres or blocks ---
    // geom::Spheres version:
    auto glass  = std::make_shared<material::Dielectric>(1.5);
    auto metal_surface = std::make_shared<material::Metal>(core::Color(0.85, 0.85, 0.95), 0.03);
    auto diffuse = std::make_shared<material::Lambertian>(core::Color(0.8, 0.3, 0.1));

    world.Add(std::make_shared<geom::Sphere>(core::Point3(3.2, 1.0, 7.0), 1.0, diffuse));
    world.Add(std::make_shared<geom::Sphere>(core::Point3(7.0, 1.0, 4.0), 1.0, metal_surface));
    world.Add(std::make_shared<geom::Sphere>(core::Point3(5.0, 1.0, 2.5), 1.0, glass));

    // Add BVH
    world_root.Add(std::make_shared<geom::BvhNode>(world.objects_, 0, world.objects_.size()));
}


void earth(scene::Scene& world) {
    auto earth_texture = std::make_shared<material::ImageTexture>("earthmap.jpg");
    auto earth_surface = std::make_shared<material::Lambertian>(earth_texture);
    auto globe = std::make_shared<geom::Sphere>(core::Point3(0,0,0), 2, earth_surface);

    world = scene::Scene(globe);
}

void Spheres(scene::Scene& world_root) {
    scene::Scene world;

    auto earth_texture = std::make_shared<material::ImageTexture>("earthmap.jpg");
    auto earth_surface = std::make_shared<material::Lambertian>(earth_texture);

    auto material_ground = std::make_shared<material::Lambertian>(core::Color(0.8, 0.8, 0.0));
    auto material_center = std::make_shared<material::Lambertian>(core::Color(0.1, 0.2, 0.5));
    auto material_left   = std::make_shared<material::Dielectric>(1.50); // model of air bubble in water
    auto material_bubble = std::make_shared<material::Dielectric>(1.00 / 1.50);
    auto material_right  = std::make_shared<material::Metal>(core::Color(0.8, 0.6, 0.2), 1.0);

    world.Add(std::make_shared<geom::Sphere>(core::Point3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.Add(std::make_shared<geom::Sphere>(core::Point3( 0.0,    0.0, -1.2),   0.5, material_center));
    world.Add(std::make_shared<geom::Sphere>(core::Point3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.Add(std::make_shared<geom::Sphere>(core::Point3(-1.0,    0.0, -1.0),   0.4, material_bubble));
    world.Add(std::make_shared<geom::Sphere>(core::Point3( 1.0,    0.0, -1.0),   0.5, material_right));

    auto checker = std::make_shared<material::CheckerTexture>(0.32, core::Color(0.2, 0.3, 0.1), core::Color(.9, .9, .9));
    world.Add(std::make_shared<geom::Sphere>(core::Point3(0,-1000,0), 1000, make_shared<material::Lambertian>(checker)));

    
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = core::RandomDouble();
            core::Point3 center(a + 0.9*core::RandomDouble(), 0.2, b + 0.9*core::RandomDouble());

            if ((center - core::Point3(4, 0.2, 0)).length() > 0.9) {
                std::shared_ptr<material::Material> sphere_material;

                if (choose_mat < 0.2) {
                    world.Add(std::make_shared<geom::Sphere>(center, 0.2, earth_surface));
                } else if(choose_mat < 0.8) {
                    // diffuse
                    auto albedo = core::RandomVec3(0, 1);
                    sphere_material = std::make_shared<material::Lambertian>(albedo);
                    world.Add(std::make_shared<geom::Sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = core::RandomVec3(0.5, 1);
                    auto fuzz = core::RandomDouble(0, 0.5);
                    sphere_material = std::make_shared<material::Metal>(albedo, fuzz);
                    world.Add(std::make_shared<geom::Sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = std::make_shared<material::Dielectric>(1.5);
                    world.Add(std::make_shared<geom::Sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = std::make_shared<material::Dielectric>(1.5);
    world.Add(std::make_shared<geom::Sphere>(core::Point3(0, 1, 0), 1.0, material1));

    auto material2 = std::make_shared<material::Lambertian>(core::Color(0.4, 0.2, 0.1));
    world.Add(std::make_shared<geom::Sphere>(core::Point3(-4, 0, 0), 1.0, material2));

    auto material3 = std::make_shared<material::Metal>(core::Color(0.7, 0.6, 0.5), 0.0);
    world.Add(std::make_shared<geom::Sphere>(core::Point3(4, 1, 0), 1.0, material3));

    //world.Add(std::make_shared<triangle>(point3(-2, -2, 0), point3(2, -2, 0), point3(0, 2, 0), material2));
    
    // auto red = std::std::make_shared<lambertian>(color(0.8,0.1,0.1));

    // auto bunny = load_obj(std::string(PROJECT_SOURCE_DIR) + "/models/stanford-bunny.obj", red, 50);

    // bvh wrapper
    // auto bunny_bvh = std::std::make_shared<bvh_node>(bunny->tris.objects, 0, bunny->tris.objects.size());

    // world.Add(bunny_bvh);

    world_root.Add(std::make_shared<geom::BvhNode>(world.objects_, 0, world.objects_.size()));
}

void checkered_spheres(scene::Scene& world) {
    auto checker = std::make_shared<material::CheckerTexture>(0.32, core::Color(.2, .3, .1), core::Color(.9, .9, .9));

    world.Add(std::make_shared<geom::Sphere>(core::Point3(0,-10, 0), 10, make_shared<material::Lambertian>(checker)));
    world.Add(std::make_shared<geom::Sphere>(core::Point3(0, 10, 0), 10, make_shared<material::Lambertian>(checker)));

    world.Add(std::make_shared<geom::BvhNode>(world.objects_, 0, world.objects_.size()));
}

int main(int argc, char** argv) {
    core::Timer clock;
    clock.reset();

    auto cameras = scene::loadCameras("cameras.json");

    std::string active = "default";
    if( argc > 1 ) {
        active = argv[1];
    }

    if( !cameras.count(active) ) {
        std::cerr << "Camera '" << active << "' not found. Using default.\n";
        active = "default";
    }
    scene::ColorCamera cam;
    cam.SetFromConfig(cameras[active]);
    cam.Initialize();

    scene::Scene world;
    switch(4) {
        case 1: Spheres(world); break;
        case 2: checkered_spheres(world); break;
        case 3: earth(world); break;
        case 4: cornell_box(world); break;
    }

    integrator::DefaultSampler default_sampler(cam.samples_per_pixel_);
    integrator::AdaptiveSampler adaptive_sampler(30, 250, 0.1f);

    //Renderer renderer(world, cam, default_sampler);

    integrator::CPURayIntegrator integrator(&world); 

    renderer::WavefrontRenderer renderer(world, cam, integrator, cam.max_depth_, cam.samples_per_pixel_, 2 * 8192);

    renderer.Render();

    std::clog << "Runtime: " << std::setprecision(2) << clock.elapsed() << "s" << std::flush;
}

