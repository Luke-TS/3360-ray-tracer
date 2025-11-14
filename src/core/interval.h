#pragma once

#include "constants.h"

#include <algorithm>

namespace rt::core {

class Interval {
public:
    double min_, max_;

    Interval() : min_(+kInfinity), max_(-kInfinity) {}

    Interval(double min, double max) : min_(min), max_(max)  {}

    // create interval overlapping two smaller intervals
    Interval(const Interval& a, const Interval& b) {
        min_ = std::min(a.min_, b.min_);
        max_ = std::max(a.max_, b.max_);
    }

    Interval Expand(double delta) const {
        auto pad = delta / 2;
        return Interval(min_ + pad, max_ + pad);
    }

    double Size() const {
        return max_ - min_;
    }

    bool Contains(double x) const {
        return min_ <= x && x <= max_;
    }

    bool Surrounds(double x) const {
        return min_ < x && x < max_;
    }

    double Clamp(double x) const {
        if (x < min_) return min_;
        if (x > max_) return max_;
        return x;
    }
};

} // namespace rt::core
