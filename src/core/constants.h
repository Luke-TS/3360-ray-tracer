#pragma once

#include <limits>

namespace rt::core {

// -----------------------------------------------------------------------------
// Fundamental mathematical constants
// -----------------------------------------------------------------------------

constexpr double kPi       = 3.14159265358979323846;
constexpr double kTwoPi    = 2.0 * kPi;
constexpr double kHalfPi   = 0.5 * kPi;
constexpr double kInvPi    = 1.0 / kPi;
constexpr double kInvTwoPi = 1.0 / (2.0 * kPi);

// -----------------------------------------------------------------------------
// Floating point limits and tolerances
// -----------------------------------------------------------------------------

// Infinity used for ray t-max, BVH bounds, etc.
constexpr double kInfinity = std::numeric_limits<double>::infinity();

// Small epsilon for intersections (prevents shadow acne)
constexpr double kEpsilon = 1e-8;

// -----------------------------------------------------------------------------
// Degree ↔ Radian conversions
// -----------------------------------------------------------------------------

constexpr double DegreesToRadians(double degrees) {
    return degrees * (kPi / 180.0);
}

constexpr double RadiansToDegrees(double radians) {
    return radians * (180.0 / kPi);
}

// -----------------------------------------------------------------------------
// Utility helpers
// -----------------------------------------------------------------------------

// Clamps value to [min, max]
constexpr double Clamp(double x, double min, double max) {
    return (x < min) ? min : (x > max) ? max : x;
}

}  // namespace rt::core
