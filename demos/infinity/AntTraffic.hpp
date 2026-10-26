#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace infinity_ants {
class Traffic {
public:
  static constexpr int side = 8, count = 16, resolution = 64;
  struct Ant {
    int x = 0, y = 0, dx = 0, dy = 0;
    float flash = 0;
  };
  std::array<Ant, count> ants{};
  uint64_t tick = 0, conflicts = 0, turns = 0;

private:
  std::array<int, side * side> visits_{};
  static int wrap(int n) { return (n + side) % side; }
  static int cell(int x, int y) { return wrap(y) * side + wrap(x); }
  static uint32_t hash(uint32_t n) {
    n ^= n >> 16;
    n *= 0x7feb352du;
    n ^= n >> 15;
    n *= 0x846ca68bu;
    return n ^ (n >> 16);
  }
  void plan() {
    std::array<bool, side * side> busy{};
    for (const auto& a : ants) busy[cell(a.x, a.y)] = true;
    constexpr int dx[] = {1, 0, -1, 0}, dy[] = {0, 1, 0, -1};
    for (int j = 0; j < count; ++j) {
      const int id = int((uint64_t(j) + tick) % count);
      auto& a = ants[id];
      int choice = -1;
      float best = 1e9f;
      a.flash = 0;
      for (int d = 0; d < 4; ++d) {
        int target = cell(a.x + dx[d], a.y + dy[d]);
        if (busy[target]) {
          ++conflicts;
          a.flash = 1;
          continue;
        }
        float score = float(visits_[target]) + (dx[d] == -a.dx && dy[d] == -a.dy ? 2.f : 0.f) +
                      float(hash(uint32_t(tick) * 131u + uint32_t(id) * 17u + uint32_t(d)) & 255u) / 512.f;
        if (score < best) {
          best = score;
          choice = d;
        }
      }
      const int nx = choice < 0 ? 0 : dx[choice], ny = choice < 0 ? 0 : dy[choice];
      if ((a.dx || a.dy) && (nx != a.dx || ny != a.dy)) ++turns;
      a.dx = nx;
      a.dy = ny;
      busy[cell(a.x + nx, a.y + ny)] = true;
    }
  }

public:
  Traffic() {
    for (int i = 0; i < count; ++i) {
      ants[i].x = (i % 4) * 2;
      ants[i].y = (i / 4) * 2;
    }
    plan();
  }
  void advance(double beat) {
    if (!std::isfinite(beat) || beat < 0) return;
    const uint64_t target = uint64_t(std::floor(std::min(beat, 1e9) * 2.0));
    if (target < tick || target - tick > 4096) {
      *this = Traffic();
      tick = target;
      plan();
      return;
    }
    while (tick < target) {
      for (auto& a : ants) {
        a.x = wrap(a.x + a.dx);
        a.y = wrap(a.y + a.dy);
        visits_[cell(a.x, a.y)] = std::min(255, visits_[cell(a.x, a.y)] + 1);
      }
      ++tick;
      if (tick % 32 == 0)
        for (auto& v : visits_) v /= 2;
      plan();
    }
  }
  using Image = std::array<float, resolution * resolution * 4>;
  void raster(double beat, Image& image) const {
    image.fill(0);
    double phase = std::isfinite(beat) ? std::fmod(std::clamp(beat, 0.0, 1e9), .5) * 2.0 : 0;
    phase = std::clamp(phase, 0.0, 1.0);
    const float t = float(phase * phase * (3 - 2 * phase));
    for (int id = 0; id < count; ++id) {
      const auto& a = ants[id];
      const float moving = (a.dx || a.dy) ? 1.f : 0.f;
      const float arc = std::sin(3.14159265f * t);
      const float cadence = std::clamp(t + arc * .22f * std::sin(float(id + 1) * 2.17f), 0.f, 1.f);
      const float wobble = arc * .65f * std::sin(float(id + 1) * 4.31f + t * 3.14f) * moving;
      const float x = (float(a.x) + float(a.dx) * cadence) * 8.f - float(a.dy) * wobble;
      const float y = (float(a.y) + float(a.dy) * cadence) * 8.f + float(a.dx) * wobble;
      for (int oy = -3; oy <= 3; ++oy)
        for (int ox = -3; ox <= 3; ++ox) {
          const int px = int(std::floor(x)) + ox, py = int(std::floor(y)) + oy;
          const float rx = float(px) + .5f - x, ry = float(py) + .5f - y;
          const float along = rx * float(a.dx) + ry * float(a.dy), across = rx * float(a.dy) - ry * float(a.dx);
          const float thorax = std::exp(-(along * along * .85f + across * across * 2.4f) * 1.15f);
          const float head = std::exp(-((along - 1.05f) * (along - 1.05f) * 1.7f + across * across * 3.f));
          const float abdomen =
              std::exp(-((along + 1.f) * (along + 1.f) * .75f + across * across * 1.75f));
          const float legs = std::exp(-std::abs(std::abs(across) - 1.15f) * 4.2f - std::abs(along) * .7f) * .16f;
          const float detailed = .52f * thorax + .32f * head + .48f * abdomen + legs;
          const float resting = std::exp(-(rx * rx + ry * ry) * 1.2f);
          const float variation = .88f + .12f * std::sin(float(id + 1) * 1.73f);
          const float body = ((a.dx || a.dy) ? detailed : resting) * variation;
          const int at = (((py + resolution) % resolution) * resolution + (px + resolution) % resolution) * 4;
          image[at] += body;
          image[at + 1] += body * a.flash * (1 - cadence);
          image[at + 2] += body * float(id) / float(count - 1);
          image[at + 3] += body;
        }
    }
  }
};
} // namespace infinity_ants
