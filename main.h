#pragma once

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>

using std::shared_ptr;
using std::make_shared;

// constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// utility functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    return std::rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) {
    return random_double()*(max-min) + min;
}

// common headers
#include "camera.h"
#include "color.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"
