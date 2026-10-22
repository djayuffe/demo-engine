#pragma once
#include "GLUtil.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

// Non-blocking query ring. Busy slots are skipped, never waited on or reused.
class InfinityGpuTimer {
  struct Slot {
    GLuint id = 0;
    bool pending = false;
    int scene = 0, frame = 0;
  };
  std::array<Slot, 8> slots_{};
  std::array<std::vector<double>, 12> ms_;
  size_t cursor_ = 0;
  int active_ = -1;
  bool enabled_ = false, report_ = false;

public:
  struct Reading {
    int frame = -1, scene = -1;
    double milliseconds = 0;
  };

private:
  Reading latest_;

public:
  const Reading& latest() const { return latest_; }
  void init(bool enabled, bool report = true) {
    enabled_ = enabled;
    report_ = report;
    if (enabled_)
      for (auto& s : slots_) glGenQueries(1, &s.id);
  }
  void poll() {
    if (!enabled_) return;
    for (auto& s : slots_)
      if (s.pending) {
        GLint ready = 0;
        glGetQueryObjectiv(s.id, GL_QUERY_RESULT_AVAILABLE, &ready);
        if (!ready) continue;
        GLuint64 ns = 0;
        glGetQueryObjectui64v(s.id, GL_QUERY_RESULT, &ns);
        if (s.frame > latest_.frame) latest_ = {s.frame, s.scene, double(ns) / 1e6};
        if (report_ && s.frame >= 10 && ms_[size_t(s.scene)].size() < 8192)
          ms_[size_t(s.scene)].push_back(double(ns) / 1e6);
        s.pending = false;
      }
  }
  void begin(int scene, int frame) {
    if (!enabled_) return;
    poll();
    auto& s = slots_[cursor_];
    active_ = -1;
    if (!s.pending) {
      s.scene = scene;
      s.frame = frame;
      glBeginQuery(GL_TIME_ELAPSED, s.id);
      active_ = int(cursor_);
    }
    cursor_ = (cursor_ + 1) % slots_.size();
  }
  void end() {
    if (active_ >= 0) {
      glEndQuery(GL_TIME_ELAPSED);
      slots_[size_t(active_)].pending = true;
      active_ = -1;
    }
  }
  void shutdown() {
    if (!enabled_) return;
    poll();
    for (size_t scene = 0; scene < ms_.size(); ++scene) {
      auto& v = ms_[scene];
      if (v.empty()) continue;
      double sum = 0;
      for (double t : v) sum += t;
      std::sort(v.begin(), v.end());
      std::printf("GPU scene %02zu: n=%zu avg=%.3f ms p95=%.3f ms\n", scene, v.size(), sum / double(v.size()),
                  v[std::min(v.size() - 1, size_t(double(v.size()) * .95))]);
    }
    for (auto& s : slots_)
      if (s.id) {
        glDeleteQueries(1, &s.id);
        s.id = 0;
      }
    enabled_ = false;
  }
};
