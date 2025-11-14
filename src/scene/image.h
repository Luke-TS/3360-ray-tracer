#pragma once
#include <string>
#include <vector>

namespace rt::scene {

class Image {
 public:
  Image() = default;
  explicit Image(const std::string& filename);

  bool Load(const std::string& filename);

  int Width() const { return width_; }
  int Height() const { return height_; }

  const unsigned char* PixelData(int x, int y) const;

 private:
  static int Clamp(int x, int low, int high);
  static unsigned char FloatToByte(float value);
  void ConvertToBytes();

  int width_ = 0;
  int height_ = 0;

  std::vector<float> fdata_;
  std::vector<unsigned char> bdata_;
};

}  // namespace rt::scene
