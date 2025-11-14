#pragma once

#include "color.h"
#include "image_loader.h"

class texture {
public:
    virtual ~texture() = default;

    virtual Color value(double u, double v, const Point3& p) const = 0;
};

class solid_color : public texture {
public:
    solid_color(const Color& albedo) : albedo(albedo) {}

    solid_color(double red, double green, double blue) : solid_color(Color(red,green,blue)) {}

    Color value(double u, double v, const Point3& p) const override {
        return albedo;
    }
private:
    Color albedo;
};

class checker_texture : public texture {
public:
    checker_texture(double scale, shared_ptr<texture> even, shared_ptr<texture> odd)
    : inv_scale(1.0 / scale), even(even), odd(odd) {}

    checker_texture(double scale, const Color& c1, const Color& c2)
    : checker_texture(scale, make_shared<solid_color>(c1), make_shared<solid_color>(c2)) {}

    Color value(double u, double v, const Point3& p) const override {
        auto xInteger = int(std::floor(inv_scale * p.x()));
        auto yInteger = int(std::floor(inv_scale * p.y()));
        auto zInteger = int(std::floor(inv_scale * p.z()));

        bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

        return isEven ? even->value(u, v, p) : odd->value(u, v, p);
    }

private:
    double inv_scale;
    shared_ptr<texture> even;
    shared_ptr<texture> odd;
};

class image_texture : public texture {
public:
    image_texture(const char* filename) : image(filename) {}

    Color value(double u, double v, const Point3& p) const override {
        // If we have no texture data, then return solid cyan as a debugging aid.
        if (image.height() <= 0) return Color(0,1,1);

        // Clamp input texture coordinates to [0,1] x [1,0]
        u = Interval(0,1).clamp(u);
        v = 1.0 - Interval(0,1).clamp(v);  // Flip V to image coordinates

        auto i = int(u * image.width());
        auto j = int(v * image.height());
        auto pixel = image.pixel_data(i,j);

        auto color_scale = 1.0 / 255.0;
        return Color(color_scale*pixel[0], color_scale*pixel[1], color_scale*pixel[2]);
    }

private:
    rtw_image image;
};
