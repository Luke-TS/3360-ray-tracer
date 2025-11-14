#define STB_IMAGE_IMPLEMENTATION
#include "third-party/stb/stb_image.h"

#include "scene/image.h"
#include <cstdlib>
#include <iostream>

namespace rt::scene {

Image::Image(const std::string& filename) {
  if (!Load(filename)) {
    std::cerr << "ERROR: Could not load image '" << filename << "'\n";
  }
}

bool Image::Load(const std::string& filename) {
    int n = 3;
    float* data = nullptr;

    // 1. Try direct path first
    data = stbi_loadf(filename.c_str(), &width_, &height_, &n, 3);

    // 2. Try project "images" directory
#ifdef IMAGE_DIR
    if (!data) {
        std::string full = std::string(IMAGE_DIR) + "/" + filename;
        data = stbi_loadf(full.c_str(), &width_, &height_, &n, 3);
    }
#endif

    // 3. Fail?
    if (!data) {
        return false;
    }

    fdata_.assign(data, data + width_ * height_ * 3);
    stbi_image_free(data);
    ConvertToBytes();

    return true;
}

void Image::ConvertToBytes() {
  bdata_.resize(width_ * height_ * 3);

  for (size_t i = 0; i < bdata_.size(); ++i)
    bdata_[i] = FloatToByte(fdata_[i]);
}

const unsigned char* Image::PixelData(int x, int y) const {
  if (bdata_.empty()) {
    static unsigned char magenta[3] = {255, 0, 255};
    return magenta;
  }

  x = Clamp(x, 0, width_);
  y = Clamp(y, 0, height_);

  return &bdata_[(y * width_ + x) * 3];
}

// helpers
int Image::Clamp(int x, int low, int high) {
  if (x < low) return low;
  if (x < high) return x;
  return high - 1;
}

unsigned char Image::FloatToByte(float value) {
  if (value <= 0.0f) return 0;
  if (value >= 1.0f) return 255;
  return static_cast<unsigned char>(value * 255.999f);
}

}  // namespace rt::scene
