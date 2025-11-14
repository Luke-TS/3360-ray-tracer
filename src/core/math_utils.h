#pragma once

#include "constants.h"
#include "random.h" 
#include "vec3.h"

#include <cmath>

namespace rt::core {

// -----------------------------------------------------------------------------
// Reflection
// -----------------------------------------------------------------------------
inline Vec3 Reflect(const Vec3& v, const Vec3& n) {
  return v - 2.0 * Dot(v, n) * n;
}

// -----------------------------------------------------------------------------
// Refraction
//   uv = unit incoming vector
//   n = surface normal
//   etai_over_etat = (n1/n2)
// -----------------------------------------------------------------------------
inline Vec3 Refract(const Vec3& uv, const Vec3& n, double etai_over_etat) {
  const double cos_theta = std::fmin(Dot(-uv, n), 1.0);
  Vec3 r_out_perp     = etai_over_etat * (uv + cos_theta * n);
  Vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;
  return r_out_perp + r_out_parallel;
}

// -----------------------------------------------------------------------------
// Uniform random vector in [0,1)^3
// -----------------------------------------------------------------------------
inline Vec3 RandomVec3() {
  return Vec3(RandomDouble(), RandomDouble(), RandomDouble());
}

// -----------------------------------------------------------------------------
// Uniform random vector in [min, max)^3
// -----------------------------------------------------------------------------
inline Vec3 RandomVec3(double min, double max) {
  return Vec3(
      RandomDouble(min, max),
      RandomDouble(min, max),
      RandomDouble(min, max)
  );
}

// -----------------------------------------------------------------------------
// Uniform Random Point in Unit Sphere
// -----------------------------------------------------------------------------
inline Vec3 RandomInUnitSphere() {
  while (true) {
    Vec3 p = RandomVec3(-1.0, 1.0);
    if (p.length_squared() < 1.0) return p;
  }
}

// -----------------------------------------------------------------------------
// Random Unit Vector (uniform direction on sphere)
// -----------------------------------------------------------------------------
inline Vec3 RandomUnitVector() {
  while (true) {
    Vec3 p = RandomVec3(-1.0, 1.0);
    const double len_sq = p.length_squared();
    if (len_sq > 1e-12 && len_sq <= 1.0) {
      return p / std::sqrt(len_sq);
    }
  }
}

// -----------------------------------------------------------------------------
// Random point on hemisphere oriented around normal
// -----------------------------------------------------------------------------
inline Vec3 RandomOnHemisphere(const Vec3& normal) {
  Vec3 on_unit_sphere = RandomUnitVector();
  return (Dot(on_unit_sphere, normal) > 0.0) ? on_unit_sphere : -on_unit_sphere;
}

// -----------------------------------------------------------------------------
// Random point in unit disk (for depth of field sampling)
// -----------------------------------------------------------------------------
inline Vec3 RandomInUnitDisk() {
  while (true) {
    Vec3 p(RandomDouble(-1, 1), RandomDouble(-1, 1), 0.0);
    if (p.length_squared() < 1.0) return p;
  }
}

// -----------------------------------------------------------------------------
// Strict normalization (safe for zero vectors)
// -----------------------------------------------------------------------------
inline Vec3 Normalize(const Vec3& v) {
  double len = v.length();
  if (len == 0.0) return Vec3(0, 0, 0);
  return v / len;
}

// -----------------------------------------------------------------------------
// Cosine-weighted hemisphere direction about a normal
//   Produces directions with PDF = cos(theta)/pi
//   Used for Lambertian importance sampling and NEE
// -----------------------------------------------------------------------------
inline Vec3 RandomCosineDirection(const Vec3& normal) {
  // Sample r1, r2 from U(0,1)
  double r1 = RandomDouble();
  double r2 = RandomDouble();

  // Convert to polar coordinates
  double phi = 2.0 * kPi * r1;
  double r   = std::sqrt(r2);
  double x   = r * std::cos(phi);
  double y   = r * std::sin(phi);
  double z   = std::sqrt(1.0 - r2);

  // Create local ONB: w = normal
  Vec3 w = Normalize(normal);
  Vec3 a = (std::fabs(w.x()) > 0.9) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
  Vec3 v = Normalize(Cross(w, a));
  Vec3 u = Cross(v, w);

  return Normalize(x * u + y * v + z * w);
}

}  // namespace rt::core
