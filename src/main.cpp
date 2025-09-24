#include "main.h"

#include "bvh.h"
#include "camera.h"
#include "material.h"
#include "hittable_list.h"
#include "sphere.h"
#include "texture.h"
#include "triangle.h"
#include "triangle_mesh.h"
#include "timer.h"
#include "load_obj.h"
#include <atomic>
#include <iomanip>
#include <iostream>
#include <memory>
#include <ostream>

void earth(hittable_list& world) {
    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0,0,0), 2, earth_surface);

    world = hittable_list(globe);

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(0,0,12);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
}

void spheres(hittable_list& world_root) {
    hittable_list world;

    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
    auto material_left   = make_shared<dielectric>(1.50); // model of air bubble in water
    auto material_bubble = make_shared<dielectric>(1.00 / 1.50);
    auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);

    world.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3( 0.0,    0.0, -1.2),   0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.4, material_bubble));
    world.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));

    auto checker = make_shared<checker_texture>(0.32, color(0.2, 0.3, 0.1), color(.9, .9, .9));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(checker)));

    
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }
    

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 0, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    //world.add(make_shared<triangle>(point3(-2, -2, 0), point3(2, -2, 0), point3(0, 2, 0), material2));
    
    // auto red = std::make_shared<lambertian>(color(0.8,0.1,0.1));

    // auto bunny = load_obj(std::string(PROJECT_SOURCE_DIR) + "/models/stanford-bunny.obj", red, 50);

    // bvh wrapper
    // auto bunny_bvh = std::make_shared<bvh_node>(bunny->tris.objects, 0, bunny->tris.objects.size());

    // world.add(bunny_bvh);

    world_root.add(make_shared<bvh_node>(world.objects, 0, world.objects.size()));
    
    camera cam;

    /*
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 20;
    cam.lookfrom = point3(-2,2,1);
    cam.lookat   = point3(0,0,-1);
    cam.vup      = vec3(0,1,0);
    */

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 1000;
    cam.samples_per_pixel = 200; 
    cam.max_depth         = 35;

    cam.vfov     = 20;
    cam.lookfrom = point3(13,5,1);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0.6;
    cam.focus_dist    = 10.0;

    //cam.render(world_root);

}

void checkered_spheres(hittable_list& world) {
    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

    world.add(make_shared<sphere>(point3(0,-10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    //cam.render(world);
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
    camera cam;
    cam.set_from_config(cameras[active]);

    hittable_list world;
    switch(1) {
        case 1: spheres(world); break;
        case 2: checkered_spheres(world); break;
        case 3: earth(world); break;
    }

    cam.render(world);

    std::clog << "Runtime: " << std::setprecision(2) << clock.elapsed() << "s" << std::flush;

    /*
    std::clog << "Num triangles: " << world_root.objects.size() << '\n';
    
    std::clog << "\nPrimitive tests: " << g_num_primitive_tests
          << "\nBox tests: " << g_num_box_tests
          << "\nAverage primitive tests per pixel: "
          << (double)g_num_primitive_tests / (cam.image_width * (cam.aspect_ratio + 1))
          << "\nAverage box tests per pixel: "
          << (double)g_num_box_tests / (cam.image_width * (cam.aspect_ratio + 1))
          << "\nAverage primitive tests per ray: "
          << (double)g_num_primitive_tests / (cam.samples_per_pixel * cam.image_width * (cam.aspect_ratio + 1))
          << "\nAverage box tests per ray: "
          << (double)g_num_box_tests / (cam.samples_per_pixel * cam.image_width * (cam.aspect_ratio + 1))
          << std::endl;
    */

}
