#pragma once

#include "main.h"
#include <algorithm>
class interval {
public:
    double min, max;

    interval() : min(-infinity) {}

    interval(double min, double max) : min(min), max(max)  {}

    // create interval overlapping two smaller intervals
    interval(const interval& a, const interval& b) {
        min = std::min(a.min, b.min);
        max = std::max(a.max, b.max);
    }

    interval expand(double delta) const {
        auto pad = delta / 2;
        return interval(min + pad, max + pad);
    }

    double size() const {
        return max - min;
    }

    bool contains(double x) const {
        return min <= x && x <= max;
    }

    bool surrounds(double x) const {
        return min < x && x < max;
    }

    double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    static const interval empty, universe;
};

const interval interval::empty = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);
