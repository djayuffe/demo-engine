#include "Slice.hpp"
#include "HyperAtlas.hpp"
#include "GpuGeometry.hpp"
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
    {
      using namespace advanced;
      GeodesicPlane plane{{std::sinh(.52), 0, 0, std::cosh(.52)}};
      const V4 origin{1, 0, 0, 0}, direction{0, 0, 0, 1};
      const double t = intersectPlane(origin, direction, plane);
      require(std::abs(t - .52) < 1e-12, "geodesic plane intersection distance");
      require(!std::isfinite(intersectPlane(origin, {0, 0, 0, -1}, plane)), "backward plane hit");
      auto hit = geodesicRay(origin, direction, t);
      require(std::abs(lorentz(hit, plane.normal)) < 1e-12, "plane hit off manifold");
      V4 tangent{};
      for (size_t i = 0; i < 4; ++i) tangent[i] = origin[i] * std::sinh(t) + direction[i] * std::cosh(t);
      auto bounced = reflectInPlane(tangent, plane), restored = reflectInPlane(bounced, plane);
      require(std::abs(lorentz(hit, bounced)) < 1e-12 && std::abs(lorentz(bounced, bounced) - 1) < 1e-12,
              "Lorentz reflection tangent invariants");
      for (size_t i = 0; i < 4; ++i)
        require(std::abs(restored[i] - tangent[i]) < 1e-12, "Lorentz reflection is not involutive");
      auto returned = geodesicRay(hit, bounced, t);
      require(h3Distance(returned, origin) < 1e-7, "mirror path did not return to origin");
      rejects([&] { intersectPlane(origin, direction, {{0, 0, 0, 0}}); });
      rejects([&] { reflectInPlane(direction, {{NAN, 0, 0, 1}}); });
    }
    const auto gpu = atlas::gpuGeometry(scene);
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
      auto node = scene.nodes[i];
      require(int(gpu.nodeData[i][1]) == node.escape && int(gpu.nodeData[i][2]) == node.leaf + 1,
              "GPU traversal metadata");
      for (size_t j = 0; j < 4; ++j)
        require(std::abs(double(gpu.nodeCenters[i][j]) - node.bound.center[j]) < 1e-6, "GPU center packing");
    }
    for (auto sphere : scene.spheres)
      for (int i = 0; i <= 100; ++i) {
        double breath = i * .01, radius = sphere.radius * (1 + .025 * breath);
        auto cached = atlas::breathingRadius(sphere.radius, breath);
        require(std::abs(cached[0] - radius) < 1e-14 && std::abs(cached[1] - std::cosh(radius)) < 1e-9 &&
                    std::abs(cached[2] - std::sinh(radius)) < 1e-9,
                "cached radius exceeds float error budget");
        require(std::abs(cached[1] * cached[1] - cached[2] * cached[2] - 1) < 1e-9, "cached hyperbolic identity");
      }
    rejects([] { atlas::breathingRadius(.2, NAN); });
    rejects([] { atlas::breathingRadius(1, 0); });
    require(gpu::specialized(1, false).find("const int uGpuCache=0;") != std::string::npos,
            "uncached GPU comparison unavailable");
    for (int step = 0; step < 100; ++step)
      for (auto point : tesseract()) {
        auto v = slice::rotated(point, step * .317);
        for (auto& x : v) x *= 1.025;
        require(std::abs(v[3]) < 3.95, "vertex cache crossed the 4D eye plane");
        auto projected = project4D(v);
        for (double x : projected) require(std::isfinite(x), "non-finite cached 4D projection");
      }

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
    const std::string shader = gpu::fragment;
    require(shader.find("vec3 atlasBackground") != std::string::npos &&
                shader.find("starLayer(direction, 260") != std::string::npos &&
                shader.find("starLayer(direction.yzx, 470") != std::string::npos,
            "Atlas celestial background layers missing");
    require(shader.find("beatWave") != std::string::npos && shader.find("* uPulse") != std::string::npos &&
                shader.find("uEye * .018") != std::string::npos,
            "Atlas background lost beat response or parallax");
    std::cout << "Atlas topology, general Lorentz camera and 12000 BVH/brute-force rays passed\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
