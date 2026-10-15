#pragma once
// Command-line options of the show layer (backgrounds, depth, gloss, pointer, clock, music start),
// parsed strictly: a number must be a whole, finite number ("2.5x" or "abc" is an error, not 0).
// Pure and unit-tested; Engine.cpp keeps the window/audio/renderer options.
#include "BackgroundStyle.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace showopt {
struct ShowOptions {
  int bgStyle = 0;              // --background-style (0..kCount-1)
  bool autoStyle = false;       // --background-style auto
  bool noBackground = false, noLogo = false, noDepth = false, noGloss = false;
  bool adaptive = true;         // --no-adaptive
  bool fixedPointer = false;    // --pointer X,Y
  float fixedX = 0.f, fixedY = 0.f;
  bool parallaxGiven = false;
  double parallax = 1.6;        // effects plane (wires, depth motes)
  double bgParallax = 1.6;      // background, independent of the effects
  double fixedStep = 0.0;       // --fixed-step S (0 = real time)
  int startTrack = 0;           // --track N (stored 0-based)
  std::string hideList;         // --hide a,b,c
};

// Strict number: the whole string must parse and be finite.
inline bool number(const char* s, double& out) {
  if (!s || !*s) return false;
  char* end = nullptr;
  const double v = std::strtod(s, &end);
  if (end == s || *end != '\0' || !std::isfinite(v)) return false;
  out = v;
  return true;
}

enum class Result { NotMine, Ok, Error };

// Parses argv[i] (and argv[i+1] when the option takes a value; i is advanced past it).
inline Result parse(int& i, int argc, char** argv, ShowOptions& o, std::string& err) {
  const std::string a = argv[i];
  const char* next = i + 1 < argc ? argv[i + 1] : nullptr;
  auto needs = [&](const char* what) -> bool {
    if (!next) { err = a + ": missing " + what; return false; }
    ++i;
    return true;
  };
  double v = 0;
  if (a == "--no-background") { o.noBackground = true; return Result::Ok; }
  if (a == "--no-logo") { o.noLogo = true; return Result::Ok; }
  if (a == "--no-depth") { o.noDepth = true; return Result::Ok; }
  if (a == "--no-gloss") { o.noGloss = true; return Result::Ok; }
  if (a == "--no-adaptive") { o.adaptive = false; return Result::Ok; }
  if (a == "--parallax" || a == "--bg-parallax") {
    if (!needs("gain 0-3")) return Result::Error;
    if (!number(argv[i], v)) { err = a + ": not a number: " + argv[i]; return Result::Error; }
    (a == "--parallax" ? o.parallax : o.bgParallax) = std::clamp(v, 0.0, 3.0);
    if (a == "--parallax") o.parallaxGiven = true;
    return Result::Ok;
  }
  if (a == "--fixed-step") {
    if (!needs("seconds per frame")) return Result::Error;
    if (!number(argv[i], v) || v <= 0 || v > 1) { err = "--fixed-step: seconds per frame in (0, 1]"; return Result::Error; }
    o.fixedStep = v;
    return Result::Ok;
  }
  if (a == "--track") {
    if (!needs("track number")) return Result::Error;
    if (!number(argv[i], v) || v < 1 || v != std::floor(v)) { err = "--track: a track number 1, 2, 3 ..."; return Result::Error; }
    o.startTrack = int(v) - 1;
    return Result::Ok;
  }
  if (a == "--pointer") {
    if (!needs("X,Y")) return Result::Error;
    const std::string s = argv[i];
    const size_t c = s.find(',');
    double x = 0, y = 0;
    if (c == std::string::npos || !number(s.substr(0, c).c_str(), x) || !number(s.substr(c + 1).c_str(), y)) {
      err = "--pointer: expected X,Y in -1..1";
      return Result::Error;
    }
    o.fixedPointer = true;
    o.fixedX = float(std::clamp(x, -1.0, 1.0));
    o.fixedY = float(std::clamp(y, -1.0, 1.0));
    return Result::Ok;
  }
  if (a == "--hide") {
    if (!needs("object list")) return Result::Error;
    o.hideList = argv[i];
    return Result::Ok;
  }
  if (a == "--background-style") {
    if (!needs("style")) return Result::Error;
    const int st = bgstyle::parse(argv[i]);
    if (st == -1) { o.autoStyle = true; return Result::Ok; }
    if (st >= 0) { o.bgStyle = st; o.autoStyle = false; return Result::Ok; }
    err = "--background-style: use auto";
    for (int k = 0; k < bgstyle::kCount; k++) err += std::string("|") + bgstyle::name(k);
    err += " (or 0-" + std::to_string(bgstyle::kCount - 1) + ")";
    return Result::Error;
  }
  return Result::NotMine;
}
}  // namespace showopt
