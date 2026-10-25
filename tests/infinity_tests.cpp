#include "Sync.hpp"
#include "Motion.hpp"
#include "OpticalPost.hpp"
#include "Parallax.hpp"
#include "OpticalMath.hpp"
#include "MatrixLife.hpp"
#include "AntTraffic.hpp"
#include "BouncingBall.hpp"
#include "SequentialRays.hpp"
#include "WorldSheets.hpp"
#include "RenderCorrections.hpp"
#include "Composition.hpp"
#include "Optimize.hpp"
#include "Shaders.hpp"
#include "Dodecahedron.hpp"
#include "../demos/cosmic/CosmicShaders.hpp"
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
int main() {
  const std::string blackHoleShader = cosmic_gpu::blackHoleFragment;
  require(blackHoleShader.find("* stepSize / max(radius * radius * radius") != std::string::npos,
          "black-hole curvature is no longer integrated per travelled distance");
  require(blackHoleShader.find("uCam * vec3(0.0, 0.0, 5.2) + uEye") != std::string::npos,
          "black-hole world-space parallax transform regressed");
  require(blackHoleShader.find("vec3 starLayer") != std::string::npos &&
              blackHoleShader.find("verticalDensity") != std::string::npos,
          "black-hole subpixel stars or volumetric disk detail missing");
  require(blackHoleShader.find("mix(pos, nextPos, crossing)") != std::string::npos,
          "black-hole disk crossing is no longer interpolated within the ray step");
  for (int scene = 0; scene < infinity_sync::sceneCount; ++scene) {
    const auto a = infinity_composition::shot(scene, 17.25, 1.2f, false);
    const auto loop = infinity_composition::shot(scene, 145.25, 1.2f, false);
    require(std::abs(a.yaw) <= .11f && std::abs(a.pitch) <= .06f && std::abs(a.eyeX) <= .07f &&
                std::abs(a.eyeY) <= .05f && a.dolly >= .95f && a.dolly <= 1.05f,
            "cinematic camera escaped its composition bounds");
    require(std::abs(a.yaw - loop.yaw) < 1e-6f && std::abs(a.pitch - loop.pitch) < 1e-6f &&
                std::abs(a.eyeX - loop.eyeX) < 1e-6f && std::abs(a.eyeY - loop.eyeY) < 1e-6f &&
                std::abs(a.dolly - loop.dolly) < 1e-6f,
            "cinematic camera phrase does not loop continuously");
  }
  const auto invalidShot = infinity_composition::shot(-9, NAN, INFINITY, false);
  require(std::isfinite(invalidShot.yaw) && std::isfinite(invalidShot.pitch) && std::isfinite(invalidShot.dolly),
          "invalid camera input escaped containment");
  {
    infinity_motion::Bands whole, split;
    const auto expected = whole.update(1, .5f, .25f, .3);
    for (int i = 0; i < 30; ++i) split.update(1, .5f, .25f, .01);
    const auto actual = split.update(1, .5f, .25f, 0);
    for (int i = 0; i < 3; ++i)
      require(std::abs(expected[i] - actual[i]) < 1e-6, "audio response depends on frame subdivision");
    const auto decay = whole.update(0, 0, 0, .3);
    require(decay[0] < expected[0] && decay[0] > 0, "audio release is not smooth");
    const auto invalid = whole.update(NAN, INFINITY, -1, NAN);
    for (int i = 0; i < 3; ++i) require(invalid[i] == decay[i], "invalid audio timestep changed state");
    infinity_motion::Clock clock;
    clock.advance(8, false, false);
    require(std::abs(clock.time() - 4.4) < 1e-12, "clock initial phase");
    clock.advance(12, true, false);
    require(std::abs(clock.time() - 4.4) < 1e-12, "pause clock drift");
    clock.advance(13, false, false);
    require(std::abs(clock.time() - 4.95) < 1e-12, "pause catchup jump");
    clock.advance(1, false, false);
    require(std::abs(clock.time() - .55) < 1e-12, "backward seek did not reset motion");
    clock.advance(NAN, false, false);
    require(std::abs(clock.time() - .55) < 1e-12, "NaN poisoned clock");
    infinity_motion::Clock reduced;
    reduced.advance(8, false, true);
    require(std::abs(reduced.time() - 4.4 * .12) < 1e-12, "reduced motion ratio");
    for (int i = 0; i < 200; ++i) {
      const auto b = infinity_motion::breath(i * .137);
      require(b >= 0 && b <= 1, "breath outside unit range");
      require(std::abs(b - infinity_motion::breath(i * .137 + 4.4)) < 1e-6, "breath period");
    }
    require(infinity_motion::breath(NAN) == 0, "invalid breath phase");
  }

  const auto checkpointShader = infinity_sequential::geometry(infinity_shader::INTEGRATE_FS, true);
  const auto fillShader = infinity_sequential::geometry(infinity_shader::INTEGRATE_FS, false);
  require(checkpointShader.find("max_vertices=16") != std::string::npos &&
              fillShader.find("max_vertices=32") != std::string::npos,
          "sequential RK4 output limits changed");
  require(checkpointShader.find("s += uDs / 6.0 * (k1 + 2.0 * k2 + 2.0 * k3 + k4);") != std::string::npos,
          "sequential pass changed imported RK4 equation");
  infinity_ants::Traffic sought;
  sought.advance(10000000);
  const auto seekTick = sought.tick;
  sought.advance(10000000.5);
  require(sought.tick == seekTick + 1, "ant colony freezes after a large seek");
  sought.advance(std::numeric_limits<double>::max());
  require(sought.tick == 2000000000ull, "ant seek clock exceeds bounded integer range");
  infinity_ball::World hugeClock;
  hugeClock.advance(std::numeric_limits<double>::max());
  const auto hugeBall = hugeClock.sample(hugeClock.hyper, std::numeric_limits<double>::max());
  for (double x : hugeBall.p) require(std::isfinite(x), "large clock corrupts ball interpolation");
  infinity_ball::Ball impact(false, true);
  impact.p = {.85, 0, 0, 0};
  impact.v = {1, 0, 0, 0};
  impact.step(.1);
  require(impact.impacts == 1 && std::abs(impact.flash - std::exp(-.09 * 16)) < 1e-8,
          "impact glow decay uses pre-hit time");
  infinity_ball::Ball corner(false, true);
  corner.p = {.85, .85, 0, 0};
  corner.v = {std::sqrt(.5), std::sqrt(.5), 0, 0};
  corner.step(.1);
  require(corner.impacts == 2 && corner.v[0] < 0 && corner.v[1] < 0, "simultaneous corner bounce incorrect");
  for (bool hyper : {false, true}) {
    infinity_ball::Ball reversible(hyper);
    const auto initial = reversible.p;
    for (int n = 0; n < 2000; ++n) reversible.step(.45 / 120);
    for (auto& x : reversible.v) x = -x;
    for (int n = 0; n < 2000; ++n) reversible.step(.45 / 120);
    for (int k = 0; k < 4; ++k)
      require(std::abs(reversible.p[k] - initial[k]) < 1e-5, "elastic dynamics are not time reversible");
  }
  for (int sample = 0; sample < 100; ++sample) {
    infinity_ball::Ball ball(true);
    ball.travel(double(sample) * .01);
    auto p = ball.p, v = ball.v;
    infinity_ball::V normal = {.3, .7, .2, 0};
    const double tangent = infinity_ball::dot(normal, p);
    for (int k = 0; k < 4; ++k) normal[k] += tangent * p[k];
    const double norm = std::sqrt(infinity_ball::dot(normal, normal));
    for (auto& x : normal) x /= norm;
    auto local = [&](const auto& x) {
      return std::array<double, 3>{x[0] - p[0] * x[3] / (p[3] + 1), x[1] - p[1] * x[3] / (p[3] + 1),
                                   x[2] - p[2] * x[3] / (p[3] + 1)};
    };
    const auto a = local(normal), b = local(v);
    double angle = 0;
    for (int k = 0; k < 3; ++k) angle += a[k] * b[k];
    require(std::abs(angle - infinity_ball::dot(normal, v)) < 1e-8, "hyperbolic local lighting changes angles");
  }
  infinity_ball::World balls;
  for (int frame = 0; frame < 24000; ++frame) {
    balls.cube.step(.45 / 120);
    balls.flat.step(.45 / 120);
    balls.hyper.step(.45 / 120);
    for (const auto* ball : {&balls.cube, &balls.flat, &balls.hyper}) {
      require(std::abs(infinity_ball::dot(ball->v, ball->v) - 1) < 1e-5, "ball loses elastic speed");
      for (int face = 0; face < (ball->cube ? 6 : 12); ++face) {
        const double d = infinity_ball::dot(ball->p, ball->normal(face));
        require(d <= (ball->hyper ? -std::sinh(ball->radius) : 1 - ball->radius) + 1e-6,
                "ball escaped chamber or penetrated wall");
      }
      if (ball->hyper) {
        require(std::abs(infinity_ball::dot(ball->p, ball->p) + 1) < 1e-6, "ball left hyperboloid");
        require(std::abs(infinity_ball::dot(ball->p, ball->v)) < 1e-6, "ball velocity left tangent space");
      }
    }
  }
  require(balls.cube.impacts > 10 && balls.flat.impacts > 10 && balls.hyper.impacts > 10, "ball never bounces");
  infinity_ball::World fastBall, slowBall;
  fastBall.advance(20);
  for (int n = 0; n <= 200; ++n) slowBall.advance(double(n) * .1);
  for (int k = 0; k < 4; ++k)
    require(std::abs(fastBall.hyper.p[k] - slowBall.hyper.p[k]) < 1e-8, "ball motion depends on frame rate");
  infinity_ball::World seekBall;
  seekBall.advance(5000);
  seekBall.advance(5000.5);
  require(seekBall.tick == 600060, "ball cannot recover after a large clock seek");
  infinity_ants::Traffic colony, stepped;
  for (int tick = 0; tick < 500; ++tick) {
    colony.advance(double(tick) * .5);
    bool source[64] = {}, target[64] = {};
    for (const auto& ant : colony.ants) {
      int a = ant.y * 8 + ant.x, b = ((ant.y + ant.dy + 8) % 8) * 8 + (ant.x + ant.dx + 8) % 8;
      require(!source[a] && !target[b], "ants overlap or reserve the same junction");
      source[a] = true;
      target[b] = true;
      require(std::abs(ant.dx) + std::abs(ant.dy) <= 1, "ant left the grid");
    }
    for (int i = 0; i < 16; ++i)
      for (int j = i + 1; j < 16; ++j) {
        const auto& a = colony.ants[i];
        const auto& b = colony.ants[j];
        for (float t : {.25f, .5f, .75f}) {
          float x = std::abs(float(a.x - b.x) + float(a.dx - b.dx) * t);
          float y = std::abs(float(a.y - b.y) + float(a.dy - b.dy) * t);
          x = std::min(x, 8.f - x);
          y = std::min(y, 8.f - y);
          require(x * x + y * y >= .49f, "ants intersect between junctions");
        }
        require(!(((a.x + a.dx + 8) % 8 == b.x) && ((a.y + a.dy + 8) % 8 == b.y) && ((b.x + b.dx + 8) % 8 == a.x) &&
                  ((b.y + b.dy + 8) % 8 == a.y)),
                "head-on ant swap");
      }
  }
  stepped.advance(249.5);
  for (int i = 0; i < 16; ++i)
    require(colony.ants[i].x == stepped.ants[i].x && colony.ants[i].y == stepped.ants[i].y &&
                colony.ants[i].dx == stepped.ants[i].dx && colony.ants[i].dy == stepped.ants[i].dy,
            "ant decisions depend on frame cadence");
  require(colony.turns > 0 && colony.conflicts > 0, "ants never react or change path");
  infinity_ants::Traffic::Image antImage;
  colony.raster(249.75, antImage);
  float total = 0;
  for (float pixel : antImage) {
    require(std::isfinite(pixel) && pixel >= 0, "invalid traffic texture");
    total += pixel;
  }
  require(total > 0, "traffic disappeared");
  const std::string shader =
      "#version 410 core\nvoid main(){float a=smoothstep(1.0,0.0,0.5);float b=smoothstep(0.0,1.0,0.5);}";
  const auto corrected = infinity_render::orderedRamps(shader);
  require(corrected.rfind("#version 410 core\n", 0) == 0, "shader version displaced");
  require(corrected.find("orderedRamp(1.0,0.0,0.5)") != std::string::npos &&
              corrected.find("orderedRamp(0.0,1.0,0.5)") != std::string::npos,
          "increasing or decreasing fade left undefined");
  require(shader.find("orderedRamp") == std::string::npos, "source shader mutated");
  require(std::isfinite(infinity_parallax::smooth(std::numeric_limits<float>::quiet_NaN(), 1, .1)),
          "invalid pointer state did not recover");
  const auto invalidEye = infinity_parallax::eye(0, std::numeric_limits<float>::infinity(), 10, true);
  require(invalidEye.x == 0 && std::abs(invalidEye.y - .16f) < 1e-6, "invalid eye coordinates not bounded");
  const auto leftEye = infinity_parallax::eye(0, -1, 0, true);
  const auto rightEye = infinity_parallax::eye(0, 1, 0, true);
  require(leftEye.x < 0 && rightEye.x > 0, "eye translation direction incorrect");
  const float nearShift = (rightEye.x - leftEye.x) / 2.f, farShift = (rightEye.x - leftEye.x) / 5.f;
  require(nearShift > farShift, "near layer must show larger parallax");
  require(infinity_parallax::eye(20, 0, 0, true).z == 0, "fixed eye still dollies");
  const float oneStep = infinity_parallax::smooth(0, 1, .1);
  const float twoSteps = infinity_parallax::smooth(infinity_parallax::smooth(0, 1, .05), 1, .05);
  require(std::abs(oneStep - twoSteps) < 1e-6, "pointer smoothing depends on frame rate");
  for (int step = 0; step < 80; ++step) {
    const auto cam = infinity_math::camera(float(step) * .31f, float(step - 40) * .03f);
    for (int a = 0; a < 3; ++a)
      for (int b = 0; b < 3; ++b) {
        float dot = 0;
        for (int k = 0; k < 3; ++k) dot += cam.m[a * 3 + k] * cam.m[b * 3 + k];
        require(std::abs(dot - (a == b ? 1.f : 0.f)) < 1e-5, "camera basis loses orthogonality");
      }
    for (int sheet = 0; sheet < 3; ++sheet) {
      const auto turn = infinity_math::turn(double(step) * 1e8, sheet);
      require(std::abs(turn[0] * turn[0] + turn[1] * turn[1] - 1) < 1e-6 &&
                  std::abs(turn[2] * turn[2] + turn[3] * turn[3] - 1) < 1e-6,
              "plane bend changes radius");
    }
  }
  for (const char* chamber : {infinity_shader::CHAMBER_OBJECT, infinity_shader::CHAMBER_CUBE,
                              infinity_shader::CHAMBER_FLAT, infinity_shader::CHAMBER_HYPER})
    require(infinity_matrix::expand(chamber).find("float ants=") != std::string::npos, "matrix walkers missing");
  using namespace infinity_sync;
  require(sample(31.999, 0, false, 0, 0, 0).scene == 0, "scene changes before boundary");
  require(sample(32, 0, false, 0, 0, 0).scene == 1, "bar-boundary cut missing");
  require(sample(beatsPerScene * sceneCount, 0, false, 0, 0, 0).scene == 0, "show does not wrap");
  for (int scene = 12; scene < sceneCount; ++scene)
    require(sample(double(scene) * beatsPerScene + 1, 0, false, 0, 0, 0).scene == scene, "new scene not directed");
  const auto heptagon = infinity_worlds::heptagon();
  require(std::abs(heptagon[0] * heptagon[0] - heptagon[1] * heptagon[1] - 1) < 1e-5,
          "Poincare mirrors not orthogonal to boundary");
  {
    const double center = heptagon[0], radius = heptagon[1], angle = 2 * 3.141592653589793 / 7;
    const double a = center * std::cos(angle * .5), vertex = a - std::sqrt(a * a - 1);
    const double x = vertex * std::cos(angle * .5), y = vertex * std::sin(angle * .5);
    const double cosine =
        ((x - center) * (x - center * std::cos(angle)) + y * (y - center * std::sin(angle))) / (radius * radius);
    require(std::abs(std::acos(-cosine) - 2 * 3.141592653589793 / 3) < 1e-5,
            "heptagon does not fit three polygons per vertex");
    for (int i = 0; i < 40; ++i) {
      const double px = .1 + double(i) * .01, py = .08;
      const double dx = px - center, dy = py, scale = radius * radius / (dx * dx + dy * dy);
      const double rx = center + dx * scale, ry = dy * scale;
      require(rx * rx + ry * ry < 1, "Poincare reflection escapes unit disk");
      const double backScale = radius * radius / ((rx - center) * (rx - center) + ry * ry);
      require(std::abs(center + (rx - center) * backScale - px) < 1e-6 && std::abs(ry * backScale - py) < 1e-6,
              "Poincare reflection is not involutive");
    }
  }
  for (int mode = 0; mode < 4; ++mode)
    require(infinity_worlds::specialized(mode).find("const int uMode = " + std::to_string(mode)) != std::string::npos,
            "world-sheet specialization missing");
  require(sample(32, 0, false, 0, 0, 0).fade == 0, "scene cut is not dark");
  require(sample(33, 0, false, 0, 0, 0).fade == 1, "scene does not fade in");
  require(sample(100, 1, true, 1, 1, 1, 5).scene == 5, "manual selection ignored");
  require(sample(100, 1, true, 1, 1, 1, 5).fade == 1, "manual view unexpectedly fades");
  require(sample(std::numeric_limits<double>::quiet_NaN(), 0, false, 0, 0, 0).scene == 0, "invalid clock unsafe");
  const auto quiet = sample(10, 0, true, 0, 0, 0), loud = sample(10, 1, true, 1, 1, 1);
  require(loud.energy > quiet.energy, "audio bands do not excite scene");
  require(sample(8, 0, true, 0, 0, 0).beat == 1.f, "downbeat attack missing");
  require(sample(8.5, 0, true, 0, 0, 0).beat < .02f, "beat decay washes out lights");
  require(sample(8.25, 0, false, 0, 0, 0).beat == sample(8.25, 0, true, 0, 0, 0).beat,
          "clock pulse drifts with audio availability");
  require(sample(8, 0, true, 0, 0, 0).strike > sample(9, 0, true, 0, 0, 0).strike, "bar accent missing");
  require(sample(8.5, 1, true, 1, 1, 1).beat < .02f, "sustained audio washes out beat attack");
  require(sample(8.5, 0, true, 0, 0, 0).flutter == 1.f, "offbeat transient missing");
  require(std::abs(sample(8, 0, false, 0, 0, 0).breath - sample(16, 0, false, 0, 0, 0).breath) < 1e-6,
          "organic phrase does not loop");
  float a[24], b[24];
  pulses(12.25, 1, a);
  pulses(12.25, 1, b);
  for (int i = 0; i < 24; ++i) require(a[i] == b[i] && std::isfinite(a[i]), "nondeterministic pulse");
  pulses(12.5, 1, b);
  require(b[0] > a[0], "beat fronts do not propagate");
  pulses(13, 1, b);
  require(b[0] == 0, "new beat does not launch front");
  pulses(0, 1, b);
  require(b[5] == 0, "pre-show beat emitted light");
  using namespace infinity_geometry;
  require(sizeof(vertices) / sizeof(float) == 60, "20 vertices missing");
  require(sizeof(edgeA) / sizeof(float) == 90, "30 edges missing");
  for (int i = 0; i < 12; ++i) {
    float len = 0;
    for (int j = 0; j < 3; ++j) len += faces[3 * i + j] * faces[3 * i + j];
    require(std::abs(len - 1) < 1e-6, "face normal not unit");
  }
  for (int i = 0; i < 30; ++i) {
    float len = 0;
    for (int j = 0; j < 3; ++j) {
      float d = edgeA[3 * i + j] - edgeB[3 * i + j];
      len += d * d;
    }
    require(len > 0, "degenerate edge");
  }
  for (int i = 0; i < 12; ++i)
    for (int j = i + 1; j < 12; ++j) {
      float d = 0;
      for (int k = 0; k < 3; ++k) d += faces[3 * i + k] * faces[3 * j + k];
      if (std::abs(d - 1 / std::sqrt(5.f)) < 1e-6)
        require(std::abs(d - kleinT * kleinT) < 1e-6, "hyperbolic adjacent mirrors not orthogonal");
    }
  for (int f = 0; f < 12; ++f) {
    const auto n = infinity_math::hyperplane(faces[3 * f], faces[3 * f + 1], faces[3 * f + 2], kleinT);
    auto dot = [](const auto& a, const auto& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] - a[3] * b[3]; };
    require(std::abs(dot(n, n) - 1) < 1e-5, "Lorentz mirror normal is not spacelike unit");
    std::array<float, 4> p = {0, 0, 0, 1}, reflected = p;
    const float d = dot(p, n);
    for (int k = 0; k < 4; ++k) reflected[k] -= 2 * d * n[k];
    require(std::abs(dot(reflected, reflected) + 1) < 1e-5, "hyperbolic bend leaves unit hyperboloid");
    const float back = dot(reflected, n);
    for (int k = 0; k < 4; ++k) reflected[k] -= 2 * back * n[k];
    for (int k = 0; k < 4; ++k) require(std::abs(reflected[k] - p[k]) < 1e-5, "mirror bend is not involutive");
  }
  int degrees[20] = {};
  bool edgesSeen[20][20] = {};
  auto vertexIndex = [&](const float* point) {
    for (int v = 0; v < 20; ++v) {
      float d = 0;
      for (int k = 0; k < 3; ++k) d += std::abs(point[k] - vertices[3 * v + k]);
      if (d < 1e-5) return v;
    }
    return -1;
  };
  float firstLength = 0;
  for (int e = 0; e < 30; ++e) {
    int a = vertexIndex(edgeA + 3 * e), b = vertexIndex(edgeB + 3 * e);
    require(a >= 0 && b >= 0 && a != b, "edge endpoint is not a vertex");
    require(!edgesSeen[a][b], "duplicate geometry edge");
    edgesSeen[a][b] = edgesSeen[b][a] = true;
    ++degrees[a];
    ++degrees[b];
    float d = 0;
    for (int k = 0; k < 3; ++k) {
      float x = edgeA[3 * e + k] - edgeB[3 * e + k];
      d += x * x;
    }
    if (e == 0) firstLength = d;
    require(std::abs(d - firstLength) < 1e-5, "dodecahedron edges have unequal lengths");
  }
  for (int degree : degrees) require(degree == 3, "dodecahedron vertex valence must be three");
  for (int f = 0; f < 12; ++f) {
    int count = 0;
    for (int v = 0; v < 20; ++v) {
      float d = 0;
      for (int k = 0; k < 3; ++k) d += faces[3 * f + k] * vertices[3 * v + k];
      require(d <= 1.00001f, "vertex escapes convex face plane");
      if (std::abs(d - 1) < 1e-5) ++count;
    }
    require(count == 5, "face must contain five coplanar vertices");
  }
  using namespace infinity_opt;
  for (int mask = 0; mask < 128; ++mask) {
    auto kernel = specialize(infinity_shader::TRACE_FS, mask);
    require(kernel.find("uniform int uMedium;") == std::string::npos, "specialization kept medium branch uniform");
    require(kernel.find("const int uHole = " + std::string(mask & 2 ? "1;" : "0;")) != std::string::npos,
            "incorrect horizon specialization");
    require(kernel.find(mask & 32 ? "const float uNout = 1.33;" : "const float uNout = 1.0;") != std::string::npos,
            "incorrect surround specialization");
  }
  require(stateForScene(4) == 0 && stateForScene(5) == 5 && stateForScene(7) == 69 && stateForScene(8) == 45,
          "preset states changed");
  require(stateForScene(9, 127) == 127, "optical override dropped");
  require(defaultPixelCeiling(false) == 400000 && defaultPixelCeiling(true) == 2000000,
          "platform pixel ceilings changed");
  require(maximumRenderScale(false) == 1.0 && maximumRenderScale(true) == 1.5,
          "platform supersample limits changed");
  require(fitPixelBudget(1280, 720, 1000000) == RenderSize{1280, 720}, "native target was downscaled");
  const auto fitted = fitPixelBudget(1920, 1080, 900000);
  require(fitted.width * fitted.height <= 900000, "fitted target exceeds pixel ceiling");
  require(std::abs(double(fitted.width) / fitted.height - 16.0 / 9.0) < .003,
          "fitted target changed aspect ratio");
  require(fitPixelBudget(0, -4, 0) == RenderSize{1, 1}, "invalid framebuffer size was not contained");
  const auto supersampled = fitPixelBudget(1280, 720, 4000000, 1.5);
  require(supersampled == RenderSize{1920, 1080}, "Metal supersample ceiling was not honored");
  require(fitPixelBudget(1280, 720, 4000000, 5.0) == RenderSize{2560, 1440},
          "unsafe supersample request was not bounded");
  require(fitPixelBudget(1280, 720, 4000000, NAN) == RenderSize{1280, 720},
          "invalid supersample scale was not contained");
  RaySolveKey key{1.5f, .65f, 46, 1};
  require(key == RaySolveKey{1.5f, .65f, 46, 1}, "identical GPU field not reused");
  require(!(key == RaySolveKey{1.6f, .65f, 46, 1}) && !(key == RaySolveKey{1.5f, .66f, 46, 1}) &&
              !(key == RaySolveKey{1.5f, .65f, 26, 1}) && !(key == RaySolveKey{1.5f, .65f, 46, 7}),
          "ray solve cache invalidation incomplete");
  SceneQuality slow;
  for (int f = 0; f < 250; ++f) slow.observe(60, f, 16.667);
  require(slow.fraction() >= .25f && slow.fraction() < .4f, "slow GPU budget did not reduce safely");
  float held = slow.fraction();
  slow.observe(1, 249, 16.667);
  require(slow.fraction() == held, "stale GPU query changed budget");
  slow.observe(std::numeric_limits<double>::quiet_NaN(), 250, 16.667);
  require(slow.fraction() == held, "invalid GPU query changed budget");
  for (int f = 251; f < 1000; ++f) slow.observe(4, f, 16.667);
  require(slow.fraction() == 1.f, "GPU budget did not recover with headroom");
  SceneQuality stable;
  for (int f = 0; f < 100; ++f) stable.observe(16, f, 16.667);
  require(stable.fraction() == 1.f, "GPU governor oscillates near target");
  std::puts("Infinity geometry, optics, physics, traffic, clocks and GPU contracts passed");
}
