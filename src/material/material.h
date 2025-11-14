#pragma once

#include <memory>
#include "core/color.h"
#include "core/ray.h"
#include "core/vec3.h"
#include "texture.h"

namespace rt::geom { class HitRecord; }

namespace rt::material {

class Material {
public:
    virtual ~Material() = default;

    virtual bool Scatter(
        const core::Ray& r_in,
        const geom::HitRecord& rec,
        core::Color& attenuation,
        core::Ray& scattered
    ) const = 0;

    virtual bool IsSpecular() const { return false; }

    // BSDF evaluation
    virtual core::Color Eval(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const = 0;

    // PDF for sampling wi given wo
    virtual float Pdf(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const = 0;

    // Sample a direction wi according to the BSDF
    virtual bool Sample(
        const geom::HitRecord& rec,
        const core::Vec3& wo,
        core::Vec3& wi,
        float& pdf,
        core::Color& f
    ) const = 0;

    // Emissive term (default: black)
    virtual core::Color Emitted(
        double u, double v, const core::Point3& p
    ) const {
        return core::Color(0,0,0);
    }
};


class Lambertian : public Material {
public:
    explicit Lambertian(const core::Color& albedo);
    explicit Lambertian(std::shared_ptr<Texture> tex);

    bool Scatter(
        const core::Ray& r_in,
        const geom::HitRecord& rec,
        core::Color& attenuation,
        core::Ray& scattered
    ) const override;

    core::Color Eval(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const override;

    float Pdf(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const override;

    bool Sample(
        const geom::HitRecord& rec,
        const core::Vec3& wo,
        core::Vec3& wi,
        float& pdf,
        core::Color& f
    ) const override;

private:
    std::shared_ptr<Texture> tex_;
};


class Metal : public Material {
public:
    Metal(const core::Color& albedo, double fuzz);

    bool Scatter(
        const core::Ray& r_in,
        const geom::HitRecord& rec,
        core::Color& attenuation,
        core::Ray& scattered
    ) const override;

    bool IsSpecular() const override;

    core::Color Eval(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const override;

    float Pdf(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const override;

    bool Sample(
        const geom::HitRecord& rec,
        const core::Vec3& wo,
        core::Vec3& wi,
        float& pdf,
        core::Color& f
    ) const override;

private:
    core::Color albedo_;
    double fuzz_;
};


class Dielectric : public Material {
public:
    explicit Dielectric(double index);

    bool Scatter(
        const core::Ray& r_in,
        const geom::HitRecord& rec,
        core::Color& attenuation,
        core::Ray& scattered
    ) const override;

    bool IsSpecular() const override;

    core::Color Eval(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const override;

    float Pdf(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const override;

    bool Sample(
        const geom::HitRecord& rec,
        const core::Vec3& wo,
        core::Vec3& wi,
        float& pdf,
        core::Color& f
    ) const override;

private:
    double ref_idx_;

    static double Reflectance(double cosine, double ref_idx);
};


class DiffuseLight : public Material {
public:
    explicit DiffuseLight(std::shared_ptr<Texture> tex);
    explicit DiffuseLight(const core::Color& c);

    bool Scatter(
        const core::Ray& r_in,
        const geom::HitRecord& rec,
        core::Color& attenuation,
        core::Ray& scattered
    ) const override;

    bool IsSpecular() const override;

    core::Color Eval(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const override;

    float Pdf(
        const geom::HitRecord& rec,
        const core::Vec3& wi,
        const core::Vec3& wo
    ) const override;

    bool Sample(
        const geom::HitRecord& rec,
        const core::Vec3& wo,
        core::Vec3& wi,
        float& pdf,
        core::Color& f
    ) const override;

    core::Color Emitted(
        double u, double v, const core::Point3& p
    ) const override;

private:
    std::shared_ptr<Texture> emit_;
};

} // namespace rt::material
