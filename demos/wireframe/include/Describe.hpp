#pragma once
// The scrolling description line: "[NN] name - provenance * recipe NNNBPM * music credit".
#include <cstdio>
#include <string>
#include <string_view>
inline std::string describeScene(int scene, std::string_view name, std::string_view provenance,
                                 const std::string& recipe, double bpm, const std::string& credit) {
  char idx[16];
  std::snprintf(idx, sizeof idx, "[%02d]", scene);
  std::string s = std::string(idx) + " " + std::string(name) + "  -  " + std::string(provenance);
  if (!recipe.empty()) s += "  *  " + recipe;
  char b[24];
  std::snprintf(b, sizeof b, " %dBPM", int(bpm + 0.5));
  s += b;
  if (!credit.empty()) s += "  *  " + credit;
  return s;
}
