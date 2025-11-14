#pragma once

#include "main.h"

// simple 3-coordinate vector class
class Vec3 {
public:
    // array of coords
    double e[3];

    // constructors
    Vec3() : e{0,0,0} {};
    Vec3(double e0, double e1, double e2) : e{e0, e1, e2} {};

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    Vec3 operator-() const { return Vec3(-e[0], -e[1], -e[2]); }
    double operator[](int i) const { return e[i]; } // for const vec3
    double& operator[](int i) { return e[i]; }      // for non-const vec3

    Vec3& operator+=(const Vec3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    Vec3& operator*=(double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    Vec3& operator/=(double t) {
        return *this *= 1/t;
    }

    double length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    double length() const {
        return std::sqrt(length_squared());
    }

    // used for edge cases where a randomly generated vectors are near zero
    bool near_zero() {
        auto s = 1e-8;
        return (std::fabs(e[0]) < s && std::fabs(e[1]) < s && std::fabs(e[2]) < s);
    }

    static Vec3 random() {
        return Vec3(random_double(), random_double(), random_double());
    }

    static Vec3 random(int min, int max) {
        return Vec3(random_double(min,max), random_double(min,max), random_double(min,max));
    }
};

// alias for code clarity
using Point3 = Vec3;

// vector utilities

inline std::ostream& operator<<(std::ostream& out, const Vec3& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline Vec3 operator+(const Vec3& u, const Vec3& v) {
    return Vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline Vec3 operator-(const Vec3& u, const Vec3& v) {
    return Vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

inline Vec3 operator*(const Vec3& u, const Vec3& v) {
    return Vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline Vec3 operator*(double t, const Vec3& v) {
    return Vec3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline Vec3 operator*(const Vec3&v, double t) {
    return t * v;
}

inline Vec3 operator/(const Vec3& v, double t) {
    return (1/t) * v;
}

inline double dot(const Vec3& u, const Vec3& v) {
    return u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
}

inline Vec3 cross(const Vec3& u, const Vec3& v) {
    return Vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

// calculates reflected ray from an incoming ray and a surface normal
inline Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - 2*dot(v, n)*n;
}

inline Vec3 refract(const Vec3& uv, const Vec3& n, double etai_over_etat) {
    auto cos_theta = std::fmin(dot(-uv, n), 1.0);
    Vec3 r_out_perp = etai_over_etat * (uv + cos_theta*n);
    Vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}

inline Vec3 unit_vector(const Vec3& v) {
    return v / v.length();
}

inline Vec3 random_unit_vector() {
    while(true) {
        auto p = Vec3::random(-1,1);
        auto lensq = p.length_squared();

        // block small values that will normalize to 0
        if( 1e-160 < lensq && lensq <= 1 ) {
            return p / sqrt(lensq);
        }
    }
}

inline Vec3 random_on_hemisphere(const Vec3& normal) {
    Vec3 on_unit_sphere = random_unit_vector();
    if( dot(on_unit_sphere, normal) > 0.0 ) {
        return on_unit_sphere;
    }
    else {
        return -on_unit_sphere;
    }
}

inline Vec3 random_in_unit_disk() {
    while(true) {
        auto p = Vec3(random_double(-1,1), random_double(-1,1),0);
        if(p.length_squared() < 1) {
            return p;
        }
    }
}

inline Vec3 normalize(const Vec3& v) {
    double len = v.length();
    if (len == 0.0)
        return Vec3(0,0,0);
    return v / len;
}

inline Vec3 random_cosine_direction(const Vec3& normal) {
    // Sample r1, r2 uniformly
    double r1 = random_double();
    double r2 = random_double();

    double phi = 2 * pi * r1;
    double r  = sqrt(r2);
    double x  = r * cos(phi);
    double y  = r * sin(phi);
    double z  = sqrt(1 - r2);  // NOTE: this produces cos(θ)-weighted distribution

    // (x, y, z) is in local coordinates where +Z is the surface normal.
    // Build an ONB (orthonormal basis) to rotate it into world space.
    Vec3 w = unit_vector(normal);
    Vec3 a = (fabs(w.x()) > 0.9) ? Vec3(0,1,0) : Vec3(1,0,0);
    Vec3 v = unit_vector(cross(w, a));
    Vec3 u = cross(v, w);

    return normalize(x * u + y * v + z * w);
}
