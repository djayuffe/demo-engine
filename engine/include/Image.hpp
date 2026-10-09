#pragma once
#include <cstdint>
#include <string>
#include <vector>
// Minimal decoded RGBA image. Decoded from JPEG/PNG on disk via stb_image.
struct Image {
  int w=0,h=0;
  std::vector<uint8_t> rgba;          // w*h*4
  bool empty() const { return w<=0||h<=0||rgba.empty(); }
  // Decode one file from disk (JPEG/PNG). Returns false on failure.
  static bool loadFromFile(const std::string& path, Image& out);
};
// Discover logo files: every UBER_*_1920x1080.jpg in dir, sorted by name.
// (The 3 original compositions and the moodboard are excluded.)
std::vector<std::string> findLogos(const std::string& dir);
