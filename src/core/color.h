#pragma once

#include "interval.h"
#include "vec3.h"

namespace rt::core {

using Color = Vec3;

inline double linear_to_gamma(double linear_component) {
    if( linear_component > 0 ) {
        return std::sqrt(linear_component);
    }

    return 0;
}

inline void write_color(std::ostream& out, const Color& pixel_color) {
    auto r = pixel_color[0];
    auto g = pixel_color[1];
    auto b = pixel_color[2];

    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    static const Interval intensity(0.000, 0.999);
    int rbyte = int(256 * intensity.Clamp(r));
    int gbyte = int(256 * intensity.Clamp(g));
    int bbyte = int(256 * intensity.Clamp(b));

    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

inline double luminance(const Color& c) {
    return 0.2126f * c.x() + 0.7152f * c.y() + 0.0722f * c.z();
}

} // namespace rt::core
