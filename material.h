#pragma once

#include "color.h"
#include "hittable.h"
#include "ray.h"
#include "vec3.h"

/**
 * abstract class for materials defining how rays scatter when intersecting
 */
class material {
public:
    virtual ~material() = default;

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const {
        return false;
    }
};

/**
* lambertian reflection for diffuse materials
* light scatters in random direction with probably
* proportional to cos(theta) where theta is angle between ray and normal
*
* this is acomplished by selecting a point from a unit sphere centered
* about a unit normal, then forming the ray from that point originating
* at the point of intersection
*/
class lambertian : public material {
public:
    lambertian(const color& albedo) : albedo(albedo) {}

    bool scatter( const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered ) const override {
        auto scatter_direction = rec.normal + random_unit_vector();

        // catch degenerate scatter direction
        if( scatter_direction.near_zero() )
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo; 
        return true;
    }
private:
    color albedo;
};

class metal : public material {
public:
    metal(const color& albedo) : albedo(albedo) {}

    bool scatter( const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered ) const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        return true;
    }

private:
    color albedo;
};
