#pragma once

#include "core/color.h"
#include "core/interval.h"

#include "scene/image.h"
#include <memory>

namespace rt::material {

class Texture {
public:
    virtual ~Texture() = default;

    virtual core::Color Value(double u, double v, const core::Point3& p) const = 0;
};

class SolidColor : public Texture {
public:
    SolidColor(const core::Color& albedo) : albedo_(albedo) {}

    SolidColor(double red, double green, double blue) : SolidColor(core::Color(red,green,blue)) {}

    core::Color Value(double u, double v, const core::Point3& p) const override {
        return albedo_;
    }
private:
    core::Color albedo_;
};

class CheckerTexture : public Texture {
public:
    CheckerTexture(double scale, std::shared_ptr<Texture> even, std::shared_ptr<Texture> odd)
    : inv_scale_(1.0 / scale), even_(even), odd_(odd) {}

    CheckerTexture(double scale, const core::Color& c1, const core::Color& c2)
    : CheckerTexture(scale, std::make_shared<SolidColor>(c1), std::make_shared<SolidColor>(c2)) {}

    core::Color Value(double u, double v, const core::Point3& p) const override {
        auto xInteger = int(std::floor(inv_scale_ * p.x()));
        auto yInteger = int(std::floor(inv_scale_ * p.y()));
        auto zInteger = int(std::floor(inv_scale_ * p.z()));

        bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

        return isEven ? even_->Value(u, v, p) : odd_->Value(u, v, p);
    }

private:
    double inv_scale_;
    std::shared_ptr<Texture> even_;
    std::shared_ptr<Texture> odd_;
};

class ImageTexture : public Texture {
public:
    ImageTexture(const char* filename) : image_(filename) {}

    core::Color Value(double u, double v, const core::Point3& p) const override {
        // If we have no texture data, then return solid cyan as a debugging aid.
        if (image_.Height() <= 0) return core::Color(0,1,1);

        // Clamp input texture coordinates to [0,1] x [1,0]
        u = core::Interval(0,1).Clamp(u);
        v = 1.0 - core::Interval(0,1).Clamp(v);  // Flip V to image coordinates

        auto i = int(u * image_.Width());
        auto j = int(v * image_.Height());
        auto pixel = image_.PixelData(i,j);

        auto color_scale = 1.0 / 255.0;
        return core::Color(color_scale*pixel[0], color_scale*pixel[1], color_scale*pixel[2]);
    }

private:
    scene::Image image_;
};

} // namespace rt::material
