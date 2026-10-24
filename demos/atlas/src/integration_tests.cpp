#include "Slice.hpp"
#include "HyperAtlas.hpp"
#include "../AtlasShaders.hpp"
#include <iostream>
#include <random>
#include <set>
using namespace dmt;
void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
template <class F> void rejects(F f) {
  bool threw = false;
  try {
    f();
  } catch (const std::domain_error&) {
    threw = true;
  }
  require(threw, "invalid geometry accepted");
}
int main() {
  try {
    for (double extent : {.975, 1.0, 1.025}) {
      auto breathing = slice::section({0, 0, 0, 1}, 0, 0, true, extent);
      require(breathing.vertices.size() == 8, "breathing section topology");
      for (auto v : breathing.vertices)
        for (double x : v) require(std::abs(std::abs(x) - extent) < 1e-10, "breathing slice extent");
    }
    rejects([] { slice::section({0, 0, 0, 1}, 0, 0, true, 0); });
    auto cube = slice::section({0, 0, 0, 1}, 0, 0);
    require(cube.vertices.size() == 8, "central cube section");
    auto octa = slice::section({1, 1, 1, 1}, 0, 0);
    require(octa.vertices.size() == 6, "diagonal octahedron section");
    require(slice::section({0, 0, 0, 1}, 1, 0).vertices.size() == 8, "coplanar cubic cell section");
    require(slice::section({0, 0, 0, 1}, 1.1, 0).empty, "empty section");
    require(slice::section({1, 1, 0, 0}, 2, 0).empty, "zero-volume square section");
    require(slice::section({1, 1, 1, 1}, 4, 0).empty, "tangent point section");
    auto live = slice::section({.31, .48, .25, .77}, .16, .7, false);
    auto full = slice::section({.31, .48, .25, .77}, .16, .7);
    require(!live.empty && live.vertices.empty() && live.planes == full.planes, "live slice plane fast path");
    for (int step = 0; step < 250; ++step) {
      auto section = slice::section({.31, .48, .25, .77}, .16 * std::sin(step * .1), step * .013);
      require(!section.empty, "unexpected empty section");
      std::set<std::pair<int, int>> edges;
      int faces = 0;
      for (auto plane : section.planes) {
        std::vector<int> ids;
        for (size_t i = 0; i < section.vertices.size(); ++i) {
          auto v = section.vertices[i];
          double d = plane[0] * v[0] + plane[1] * v[1] + plane[2] * v[2] - plane[3];
          require(d < 1e-7, "slice outside convex bounds");
          if (std::abs(d) < 1e-7) ids.push_back(int(i));
        }
        if (ids.size() < 3) continue;
        ++faces;
        for (size_t a = 0; a < ids.size(); ++a)
          for (size_t b = a + 1; b < ids.size(); ++b) {
            int shared = 0;
            for (auto p : section.planes) {
              auto x = section.vertices[ids[a]], y = section.vertices[ids[b]];
              if (std::abs(p[0] * x[0] + p[1] * x[1] + p[2] * x[2] - p[3]) < 1e-7 &&
                  std::abs(p[0] * y[0] + p[1] * y[1] + p[2] * y[2] - p[3]) < 1e-7)
                ++shared;
            }
            if (shared >= 2) edges.insert({ids[a], ids[b]});
          }
      }
      require(int(section.vertices.size()) - int(edges.size()) + faces == 2, "slice Euler characteristic");
    }
    atlas::Atlas scene;
    require(scene.nodes.size() == 111, "complete binary sphere hierarchy");
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
      auto n = scene.nodes[i];
      require(n.escape > int(i) && n.escape <= 111, "stackless escape order");
      for (size_t j = i + 1; j < size_t(n.escape); ++j)
        if (scene.nodes[j].leaf >= 0) {
          auto s = scene.spheres[size_t(scene.nodes[j].leaf)];
          require(h3Distance(n.bound.center, s.center) + s.radius <= n.bound.radius + 1e-7,
                  "H3 bound does not contain descendant");
          require(h3Distance(n.bound.center, s.center) + s.radius * 1.025 <= n.bound.radius + .008,
                  "breathing sphere escaped GPU bound");
        }
    }
    std::mt19937 rng(2016);
    std::uniform_real_distribution<double> random(-1, 1);
    int hits = 0;
    for (int i = 0; i < 12000; ++i) {
      auto origin = h3Point(random(rng) * .7, random(rng) * .7, random(rng) * .2);
      auto d = advanced::tangentFromOrigin(origin, {random(rng), random(rng), .5 + random(rng)});
      require(std::abs(lorentz(origin, d)) < 1e-12 && std::abs(lorentz(d, d) - 1) < 1e-12, "general camera tangent");
      double a = scene.intersect(origin, d), b = scene.intersect(origin, d, false);
      require((!std::isfinite(a) && !std::isfinite(b)) || std::abs(a - b) < 1e-9, "BVH differs from brute force");
      if (std::isfinite(a)) ++hits;
    }
    require(hits > 500, "insufficient ray coverage");
    for (int i = 0; i < 4; ++i)
      for (int j = i + 1; j < 4; ++j) {
        V4 a{.2, .3, .4, .5};
        auto b = rotatePlane(a, i, j, .7);
        require(std::abs(slice::dot4(a, a) - slice::dot4(b, b)) < 1e-12, "4D rotation norm");
      }
    rejects([] { slice::section({0, 0, 0, 0}, 0, 0); });
    rejects([] { advanced::normalized({NAN, 0, 0}); });
    rejects([] { advanced::tangentFromOrigin({NAN, 0, 0, 0}, {0, 0, 1}); });
    rejects([] { rotatePlane({1, 0, 0, 0}, 0, 4, 0); });
    rejects([] { diskDistance({1, 0}, {0, 0}); });
    rejects([] { project4D({0, 0, 0, 4}); });
    for (int i = 0; i < gpu::modes; ++i)
      require(gpu::specialized(i).find("const int uMode=" + std::to_string(i)) != std::string::npos,
              "GPU specialization");
    std::cout << "Atlas topology, general Lorentz camera and 12000 BVH/brute-force rays passed\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
