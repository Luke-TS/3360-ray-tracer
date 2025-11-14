#pragma once

#include <cmath>
#include <iostream>

namespace rt::core {

class Vec3 {
 public:
  constexpr Vec3() : e_{0, 0, 0} {}
  constexpr Vec3(double x, double y, double z) : e_{x, y, z} {}

  // Accessors
  constexpr double x() const { return e_[0]; }
  constexpr double y() const { return e_[1]; }
  constexpr double z() const { return e_[2]; }

  constexpr double operator[](int i) const { return e_[i]; }
  double& operator[](int i) { return e_[i]; }

  // Unary minus
  constexpr Vec3 operator-() const { return Vec3(-e_[0], -e_[1], -e_[2]); }

  // Compound operations
  Vec3& operator+=(const Vec3& v) {
    e_[0] += v.e_[0];
    e_[1] += v.e_[1];
    e_[2] += v.e_[2];
    return *this;
  }

  Vec3& operator*=(double t) {
    e_[0] *= t;
    e_[1] *= t;
    e_[2] *= t;
    return *this;
  }

  Vec3& operator/=(double t) {
    return *this *= (1.0 / t);
  }

  // Norms
  double length() const { return std::sqrt(length_squared()); }

  constexpr double length_squared() const {
    return e_[0] * e_[0] + e_[1] * e_[1] + e_[2] * e_[2];
  }

  // Near-zero check
  bool NearZero() const {
    constexpr double s = 1e-8;
    return (std::fabs(e_[0]) < s &&
            std::fabs(e_[1]) < s &&
            std::fabs(e_[2]) < s);
  }

 private:
  double e_[3];
};

// Type alias for points in 3D
using Point3 = Vec3;

// Free functions --------------------------

inline std::ostream& operator<<(std::ostream& out, const Vec3& v) {
  return out << v[0] << ' ' << v[1] << ' ' << v[2];
}

inline Vec3 operator+(const Vec3& u, const Vec3& v) {
  return Vec3(u[0] + v[0], u[1] + v[1], u[2] + v[2]);
}

inline Vec3 operator-(const Vec3& u, const Vec3& v) {
  return Vec3(u[0] - v[0], u[1] - v[1], u[2] - v[2]);
}

inline Vec3 operator*(const Vec3& u, const Vec3& v) {
  return Vec3(u[0] * v[0], u[1] * v[1], u[2] * v[2]);
}

inline Vec3 operator*(double t, const Vec3& v) {
  return Vec3(t * v[0], t * v[1], t * v[2]);
}

inline Vec3 operator*(const Vec3& v, double t) {
  return t * v;
}

inline Vec3 operator/(const Vec3& v, double t) {
  return (1.0 / t) * v;
}

inline double Dot(const Vec3& u, const Vec3& v) {
  return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
}

inline Vec3 Cross(const Vec3& u, const Vec3& v) {
  return Vec3(
      u[1] * v[2] - u[2] * v[1],
      u[2] * v[0] - u[0] * v[2],
      u[0] * v[1] - u[1] * v[0]
  );
}

}  // namespace rt::core
