#pragma once

#include "main.h"
#include <algorithm>
class Interval {
public:
    double min, max;

    Interval() : min(+infinity), max(-infinity) {}

    Interval(double min, double max) : min(min), max(max)  {}

    // create interval overlapping two smaller intervals
    Interval(const Interval& a, const Interval& b) {
        min = std::min(a.min, b.min);
        max = std::max(a.max, b.max);
    }

    Interval expand(double delta) const {
        auto pad = delta / 2;
        return Interval(min + pad, max + pad);
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

    static const Interval empty, universe;
};

const Interval Interval::empty = Interval(+infinity, -infinity);
const Interval Interval::universe = Interval(-infinity, +infinity);
