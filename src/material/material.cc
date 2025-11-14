#include "material.h"
#include "geom/hittable.h"
#include "core/random.h"
#include "core/constants.h"
#include "core/math_utils.h"
#include "texture.h"

#include <cmath>

namespace rt::material {

// ---------------- Lambertian ----------------

Lambertian::Lambertian(const core::Color& albedo)
    : tex_(std::make_shared<SolidColor>(albedo)) {}

Lambertian::Lambertian(std::shared_ptr<Texture> tex)
    : tex_(std::move(tex)) {}

bool Lambertian::Scatter(
    const core::Ray& r_in,
    const geom::HitRecord& rec,
    core::Color& attenuation,
    core::Ray& scattered
) const {
    core::Vec3 scatter_dir = rec.normal + core::RandomUnitVector();

    if (scatter_dir.NearZero())
        scatter_dir = rec.normal;

    scattered = core::Ray(rec.p, scatter_dir);
    attenuation = tex_->Value(rec.u, rec.v, rec.p);
    return true;
}

core::Color Lambertian::Eval(
    const geom::HitRecord& rec,
    const core::Vec3& wi,
    const core::Vec3& /*wo*/
) const {
    if (core::Dot(rec.normal, wi) <= 0)
        return core::Color(0,0,0);

    core::Color albedo = tex_->Value(rec.u, rec.v, rec.p);
    return albedo / core::kPi;
}

float Lambertian::Pdf(
    const geom::HitRecord& rec,
    const core::Vec3& wi,
    const core::Vec3& /*wo*/
) const {
    float cosTheta = core::Dot(rec.normal, wi);
    return (cosTheta <= 0.0f) ? 0.0f : cosTheta / core::kPi;
}

bool Lambertian::Sample(
    const geom::HitRecord& rec,
    const core::Vec3& wo,
    core::Vec3& wi,
    float& pdf,
    core::Color& f
) const {
    wi = core::RandomCosineDirection(rec.normal);
    pdf = Pdf(rec, wi, wo);
    f = Eval(rec, wi, wo);
    return true;
}


// ---------------- Metal ----------------

Metal::Metal(const core::Color& albedo, double fuzz)
    : albedo_(albedo)
    , fuzz_(fuzz < 1.0 ? fuzz : 1.0) {}

bool Metal::Scatter(
    const core::Ray& r_in,
    const geom::HitRecord& rec,
    core::Color& attenuation,
    core::Ray& scattered
) const {
    core::Vec3 reflected = core::Reflect(r_in.direction(), rec.normal);
    reflected += fuzz_ * core::RandomUnitVector();

    scattered = core::Ray(rec.p, reflected);
    attenuation = albedo_;
    return core::Dot(scattered.direction(), rec.normal) > 0;
}

bool Metal::IsSpecular() const {
    return true;
}

core::Color Metal::Eval(
    const geom::HitRecord&,
    const core::Vec3&,
    const core::Vec3&
) const {
    // delta distribution
    return core::Color(0,0,0);
}

float Metal::Pdf(
    const geom::HitRecord&,
    const core::Vec3&,
    const core::Vec3&
) const {
    return 0.0f; // delta
}

bool Metal::Sample(
    const geom::HitRecord& rec,
    const core::Vec3& wo,
    core::Vec3& wi,
    float& pdf,
    core::Color& f
) const {
    wi = core::Reflect(-wo, rec.normal);
    wi += fuzz_ * core::RandomUnitVector();

    if (core::Dot(wi, rec.normal) <= 0)
        return false;

    pdf = 1.0f;    // handled as delta
    f = albedo_;
    return true;
}


// ---------------- Dielectric ----------------

Dielectric::Dielectric(double index)
    : ref_idx_(index) {}

bool Dielectric::Scatter(
    const core::Ray& r_in,
    const geom::HitRecord& rec,
    core::Color& attenuation,
    core::Ray& scattered
) const {
    attenuation = core::Color(1.0, 1.0, 1.0);

    double eta = rec.front_face ? (1.0 / ref_idx_) : ref_idx_;
    core::Vec3 unit_dir = core::Normalize(r_in.direction());

    double cos_theta = std::fmin(core::Dot(-unit_dir, rec.normal), 1.0);
    double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

    bool cannot_refract = eta * sin_theta > 1.0;
    core::Vec3 direction;

    if (cannot_refract || Reflectance(cos_theta, eta) > core::RandomDouble())
        direction = core::Reflect(unit_dir, rec.normal);
    else
        direction = core::Refract(unit_dir, rec.normal, eta);

    scattered = core::Ray(rec.p, direction);
    return true;
}

bool Dielectric::IsSpecular() const {
    return true;
}

core::Color Dielectric::Eval(
    const geom::HitRecord&,
    const core::Vec3&,
    const core::Vec3&
) const {
    return core::Color(0,0,0);
}

float Dielectric::Pdf(
    const geom::HitRecord&,
    const core::Vec3&,
    const core::Vec3&
) const {
    return 0.0f;
}

bool Dielectric::Sample(
    const geom::HitRecord& rec,
    const core::Vec3& wo,
    core::Vec3& wi,
    float& pdf,
    core::Color& f
) const {
    f = core::Color(1.0, 1.0, 1.0);
    pdf = 1.0f;

    double eta = rec.front_face ? (1.0 / ref_idx_) : ref_idx_;
    core::Vec3 unit_wo = core::Normalize(wo);

    double cos_theta = std::fmin(core::Dot(-unit_wo, rec.normal), 1.0);
    double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

    bool cannot_refract = eta * sin_theta > 1.0;

    if (cannot_refract || core::RandomDouble() < Reflectance(cos_theta, eta))
        wi = core::Reflect(-unit_wo, rec.normal);
    else
        wi = core::Refract(-unit_wo, rec.normal, eta);

    return true;
}

double Dielectric::Reflectance(double cosine, double ref_idx) {
    double r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * std::pow(1.0 - cosine, 5.0);
}


// ---------------- DiffuseLight ----------------

DiffuseLight::DiffuseLight(std::shared_ptr<Texture> tex)
    : emit_(std::move(tex)) {}

DiffuseLight::DiffuseLight(const core::Color& c)
    : emit_(std::make_shared<SolidColor>(c)) {}

bool DiffuseLight::Scatter(
    const core::Ray&,
    const geom::HitRecord&,
    core::Color&,
    core::Ray&
) const {
    return false; // emissive only, no scattering
}

bool DiffuseLight::IsSpecular() const {
    return true; // treated specially
}

core::Color DiffuseLight::Eval(
    const geom::HitRecord&,
    const core::Vec3&,
    const core::Vec3&
) const {
    return core::Color(0,0,0);
}

float DiffuseLight::Pdf(
    const geom::HitRecord&,
    const core::Vec3&,
    const core::Vec3&
) const {
    return 0.0f;
}

bool DiffuseLight::Sample(
    const geom::HitRecord&,
    const core::Vec3&,
    core::Vec3&,
    float&,
    core::Color&
) const {
    return false;
}

core::Color DiffuseLight::Emitted(
    double u, double v, const core::Point3& p
) const {
    return emit_->Value(u, v, p);
}

} // namespace rt::material
