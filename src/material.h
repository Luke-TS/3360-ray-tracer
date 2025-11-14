#pragma once

#include "color.h"
#include "hittable.h"
#include "ray.h"
#include "vec3.h"
#include "texture.h"

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

    virtual bool is_specular() const { return false; }

    // Next-event estimation / MIS interface:

    // Evaluate BSDF f_r(wi,wo)
    virtual color eval(
        const hit_record& rec,
        const vec3& wi,
        const vec3& wo
    ) const = 0;

    // PDF of sampling wi
    virtual float pdf(
        const hit_record& rec,
        const vec3& wi,
        const vec3& wo
    ) const = 0;

    // Sample a direction wi according to BSDF
    virtual bool sample(
        const hit_record& rec,
        const vec3& wo,
        vec3& wi,
        float& pdf,
        color& f
    ) const = 0;

    virtual color emitted(double u, double v, const point3& p) const {
        return color(0,0,0);
    };
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
    lambertian(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}
    lambertian(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter( const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered ) const override {
        auto scatter_direction = rec.normal + random_unit_vector();

        // catch degenerate scatter direction
        if( scatter_direction.near_zero() )
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction);
        attenuation = tex->value(rec.u, rec.v, rec.p); 
        return true;
    }

    // NEE / MIS interface

    // Evaluate BSDF
    color eval(const hit_record& rec, const vec3& wi, const vec3& wo) const override {
        if (dot(rec.normal, wi) <= 0) return color(0,0,0);

        color albedo = tex->value(rec.u, rec.v, rec.p);
        return albedo / pi;
    }

    // PDF = cos(theta) / PI
    float pdf(const hit_record& rec, const vec3& wi, const vec3& wo) const override {
        float cosTheta = dot(rec.normal, wi);
        return (cosTheta <= 0) ? 0.0f : (cosTheta / pi);
    }

    // Sample according to cosine-weighted hemisphere
    bool sample(const hit_record& rec, const vec3& wo, vec3& wi, float& pdf_val, color& f) const override {
        wi = random_cosine_direction(rec.normal);
        pdf_val = pdf(rec, wi, wo);
        f = eval(rec, wi, wo);
        return true;
    }

private:
    shared_ptr<texture> tex;
};

/*
* metal objects scatter rays using mirrored reflection
*
* also refered to as specular materials
*
* a random unit vector is added to the reflected ray, scaled by fuzz
* fuzz controls how 'polished' the metal appears
* fuzz == 0 --> perfect reflections (mirror)
* fuzz >  0 --> reflections slightly random, appears slightly blurred
* fuzz == 1 --> rough metal, reflections heavily smeared
*/
class metal : public material {
public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter( const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered ) const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = reflected + (fuzz * random_unit_vector());
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

    bool is_specular() const override { return true; }

    color eval(const hit_record&, const vec3&, const vec3&) const override {
        return color(0,0,0); // delta distribution
    }

    float pdf(const hit_record&, const vec3&, const vec3&) const override {
        return 0.0f; // delta
    }

    bool sample(const hit_record& rec, const vec3& wo, vec3& wi, float& pdf_val, color& f) const override {
        wi = reflect(-wo, rec.normal);
        wi += fuzz * random_unit_vector();

        if (dot(wi, rec.normal) <= 0) return false;

        pdf_val = 1.0f;  // delta treated specially
        f = albedo;      // reflectance
        return true;
    }

private:
    color albedo;
    double fuzz;
};

class dielectric : public material {
public:
    dielectric(double refraction_index) : ref_idx(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        attenuation = color(1.0, 1.0, 1.0);
        double ri = rec.front_face ? (1.0/ref_idx) : ref_idx;

        vec3 unit_direction = unit_vector(r_in.direction());

        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);

        scattered = ray(rec.p, direction);
        return true;
    }

    bool is_specular() const override { return true; }

    color eval(
        const hit_record&, const vec3&, const vec3&
    ) const override { return color(0,0,0); }

    float pdf(
        const hit_record&, const vec3&, const vec3&
    ) const override { return 0.0f; }

    bool sample(
        const hit_record& rec,
        const vec3& wo,
        vec3& wi,
        float& pdf_val,
        color& f
    ) const override {

        // Always white (glass does not tint light)
        f = color(1,1,1);
        pdf_val = 1.0f;

        double eta = rec.front_face ? (1.0 / ref_idx) : ref_idx;

        vec3 unit_wo = unit_vector(wo);
        double cos_theta = fmin(dot(-unit_wo, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = eta * sin_theta > 1.0;

        // Schlick reflectance
        double reflect_prob = reflectance(cos_theta, eta);

        if (cannot_refract || random_double() < reflect_prob) {
            wi = reflect(-unit_wo, rec.normal);
        } else {
            wi = refract(-unit_wo, rec.normal, eta);
        }

        return true;
    }

private:
    double ref_idx;

    static double reflectance(double cosine, double refraction_index) {
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0 * r0;
        return r0 + (1-r0)*std::pow((1-cosine), 5);
    }
};

class diffuse_light : public material {
public:
    diffuse_light(shared_ptr<texture> tex) : emit(tex) {}
    diffuse_light(const color& c) : emit(make_shared<solid_color>(c)) {}

    // Emissive surfaces don't scatter; they just emit
    bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const override {
        return false;
    }

    bool is_specular() const override { return true; }


    bool sample( const hit_record&, const vec3&, vec3&, float&, color& ) const override {
        return false; // no scattering
    }

    float pdf( const hit_record&, const vec3&, const vec3&) const override { return 0.0f; }

    color eval( const hit_record&, const vec3&, const vec3&) const override { return color(0,0,0); }

    // Return emitted radiance (light) at hit point
    color emitted(double u, double v, const point3& p) const override {
        return emit->value(u, v, p);
    }

private:
    shared_ptr<texture> emit;
};
