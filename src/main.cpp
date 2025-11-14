#include "main.h"

#include "bvh.h"
#include "camera.h"
#include "material.h"
#include "scene.h"
#include "Sampler.h"
#include "sphere.h"
#include "rect.h"
#include "gpu_utils.h"
#include "texture.h"
#include "timer.h"
#include "wavefront_renderer.h"
#include "cpu_ray_integrator.h"

#include <iomanip>
#include <iostream>
#include <ostream>

void flatten_bvh_debug(Scene& scene) {
    auto world_bvh = std::make_shared<BvhNode>(scene);
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

void cornell_box(Scene& world_root) {
    Scene world;

    // Materials
    auto red   = make_shared<lambertian>(Color(.65, .05, .05));
    auto white = make_shared<lambertian>(Color(.73, .73, .73));
    auto green = make_shared<lambertian>(Color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(Color(15, 15, 15));  // Brighter light

    // Standard Cornell Box scaling (10×10×10 world units)
    const double S = 10.0;       // side length
    const double eps = 0.01;     // small offset to avoid z-fighting

    // --- Walls ---
    world.add(make_shared<yz_rect>(0, S, 0, S,  S, green));   // Right wall
    world.add(make_shared<yz_rect>(0, S, 0, S,  0, red));     // Left wall
    world.add(make_shared<xz_rect>(0, S, 0, S,  0, white));   // Floor
    world.add(make_shared<xz_rect>(0, S, 0, S,  S, white));   // Ceiling
    world.add(make_shared<xy_rect>(0, S, 0, S,  S, white));   // Back wall

    // --- Light in ceiling ---
    // Standard proportion: light is ~1/3 width of box
    const double L0 = 3.0;
    const double L1 = 7.0;
    world.add(make_shared<xz_rect>(L0, L1, L0, L1, S - eps, light));  

    // --- Objects: Use spheres or blocks ---
    // Spheres version:
    auto glass  = make_shared<dielectric>(1.5);
    auto metal_surface = make_shared<metal>(Color(0.85, 0.85, 0.95), 0.03);
    auto diffuse = make_shared<lambertian>(Color(0.8, 0.3, 0.1));

    world.add(make_shared<Sphere>(Point3(3.2, 1.0, 7.0), 1.0, diffuse));
    world.add(make_shared<Sphere>(Point3(7.0, 1.0, 4.0), 1.0, metal_surface));
    world.add(make_shared<Sphere>(Point3(5.0, 1.0, 2.5), 1.0, glass));

    // Add BVH
    world_root.add(make_shared<BvhNode>(world.objects, 0, world.objects.size()));
}


void earth(Scene& world) {
    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<Sphere>(Point3(0,0,0), 2, earth_surface);

    world = Scene(globe);
}

void spheres(Scene& world_root) {
    Scene world;

    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);

    auto material_ground = make_shared<lambertian>(Color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(Color(0.1, 0.2, 0.5));
    auto material_left   = make_shared<dielectric>(1.50); // model of air bubble in water
    auto material_bubble = make_shared<dielectric>(1.00 / 1.50);
    auto material_right  = make_shared<metal>(Color(0.8, 0.6, 0.2), 1.0);

    world.add(make_shared<Sphere>(Point3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<Sphere>(Point3( 0.0,    0.0, -1.2),   0.5, material_center));
    world.add(make_shared<Sphere>(Point3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(make_shared<Sphere>(Point3(-1.0,    0.0, -1.0),   0.4, material_bubble));
    world.add(make_shared<Sphere>(Point3( 1.0,    0.0, -1.0),   0.5, material_right));

    auto checker = make_shared<checker_texture>(0.32, Color(0.2, 0.3, 0.1), Color(.9, .9, .9));
    world.add(make_shared<Sphere>(Point3(0,-1000,0), 1000, make_shared<lambertian>(checker)));

    
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            Point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - Point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<Material> sphere_material;

                if (choose_mat < 0.2) {
                    world.add(make_shared<Sphere>(center, 0.2, earth_surface));
                } else if(choose_mat < 0.8) {
                    // diffuse
                    auto albedo = Color::random() * Color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<Sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = Color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<Sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<Sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<Sphere>(Point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(Color(0.4, 0.2, 0.1));
    world.add(make_shared<Sphere>(Point3(-4, 0, 0), 1.0, material2));

    auto material3 = make_shared<metal>(Color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<Sphere>(Point3(4, 1, 0), 1.0, material3));

    //world.add(make_shared<triangle>(point3(-2, -2, 0), point3(2, -2, 0), point3(0, 2, 0), material2));
    
    // auto red = std::make_shared<lambertian>(color(0.8,0.1,0.1));

    // auto bunny = load_obj(std::string(PROJECT_SOURCE_DIR) + "/models/stanford-bunny.obj", red, 50);

    // bvh wrapper
    // auto bunny_bvh = std::make_shared<bvh_node>(bunny->tris.objects, 0, bunny->tris.objects.size());

    // world.add(bunny_bvh);

    world_root.add(make_shared<BvhNode>(world.objects, 0, world.objects.size()));
}

void checkered_spheres(Scene& world) {
    auto checker = make_shared<checker_texture>(0.32, Color(.2, .3, .1), Color(.9, .9, .9));

    world.add(make_shared<Sphere>(Point3(0,-10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<Sphere>(Point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    world.add(make_shared<BvhNode>(world.objects, 0, world.objects.size()));
}

int main(int argc, char** argv) {
    Timer clock;
    clock.reset();

    auto cameras = loadCameras("cameras.json");

    std::string active = "default";
    if( argc > 1 ) {
        active = argv[1];
    }

    if( !cameras.count(active) ) {
        std::cerr << "Camera '" << active << "' not found. Using default.\n";
        active = "default";
    }
    color_camera cam;
    cam.set_from_config(cameras[active]);
    cam.initialize();

    Scene world;
    switch(1) {
        case 1: spheres(world); break;
        case 2: checkered_spheres(world); break;
        case 3: earth(world); break;
        case 4: cornell_box(world); break;
    }

    DefaultSampler default_sampler(cam.samples_per_pixel);
    AdaptiveSampler adaptive_sampler(30, 250, 0.1f);

    //Renderer renderer(world, cam, default_sampler);

    CPURayIntegrator integrator(&world); 

    WavefrontRenderer renderer(world, cam, integrator, cam.max_depth, cam.samples_per_pixel, 2 * 8192);

    renderer.render();

    std::clog << "Runtime: " << std::setprecision(2) << clock.elapsed() << "s" << std::flush;
}
