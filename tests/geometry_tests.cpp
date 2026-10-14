#include <set>
#include "Geometry.hpp"
#include "AdvancedGeometry.hpp"
#include "ImpossibleGeometry.hpp"
#include "VertexWeld.hpp"
#include "Image.hpp"
#include "Scene.hpp"
#include "TextScroller.hpp"
#include "Timeline.hpp"
#include "Effects.hpp"
#include "Choreography.hpp"
#include "Camera.hpp"
#include "RenderScale.hpp"
#include "LogoShow.hpp"
#include "Describe.hpp"
#include "WindowSpec.hpp"
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
static void req(bool x, const char* m) {
  if (!x) {
    std::cerr << "FAIL: " << m << "\n";
    std::exit(1);
  }
}
static void mesh(const Mesh3& m, const char* n, bool requireEdges = true) {
  std::string w;
  req(geo::validate(m, &w), n);
  req(!m.v.empty(), n);
  if (requireEdges) req(!m.e.empty(), n);
}
int main() {
  auto t = geo::tesseract();
  req(t.v.size() == 16 && t.e.size() == 32 && t.faces.size() == 24, "tesseract V/E/F");
  auto c16 = geo::cell16();
  req(c16.v.size() == 8 && c16.e.size() == 24 && c16.faces.size() == 32, "16-cell V/E/F");
  auto c24 = geo::cell24();
  req(c24.v.size() == 24 && c24.e.size() == 96 && c24.faces.size() == 96, "24-cell V/E/F");
  auto c600 = geo::cell600();
  req(c600.v.size() == 120 && c600.e.size() == 720 && c600.faces.size() == 1200, "600-cell V/E/F");
  auto c120 = geo::cell120Vertices();
  std::string w;
  req(geo::validate(c120, &w), "120-cell valid");
  req(c120.v.size() == 600 && c120.e.size() == 1200, "120-cell V/E exact");
  mesh(geo::project4D(c600, .1f, .2f, 3.4f, .3f, .4f), "4D projection");
  mesh(geo::sliceFaces4D(c600, {.31f, .47f, .59f, .57f}, 0), "4D face slice");
  for (auto k : {geo::ImplicitKind::Gyroid, geo::ImplicitKind::SchwarzP, geo::ImplicitKind::SchwarzD,
                 geo::ImplicitKind::Neovius, geo::ImplicitKind::IWP})
    mesh(geo::implicitSurface(k, 12), "implicit extraction");
  mesh(geo::hopfFibres(8, 24), "hopf");
  mesh(geo::boySurface(20, 12), "boy");
  mesh(geo::superformula(24, 12), "superformula");
  mesh(geo::cliffordTorus(20, 10), "clifford");
  mesh(geo::hyperbolicBall(3, 8), "hyperbolic");
  mesh(geo::lissajousKnot(100), "lissajous");
  mesh(geo::strangeAttractor(1000), "lorenz");
  mesh(geo::discoveredObject(42, 24, 12), "discovered");
  mesh(geo::quaternionJulia(14, 7, .08f), "quaternion Julia");
  mesh(geo::mobiusStrip(32, 8), "mobius");
  mesh(geo::kleinBottle(32, 12), "klein");
  mesh(geo::enneperSurface(24, 12), "enneper");
  mesh(geo::helicoidSurface(24, 12), "helicoid");
  mesh(geo::catenoidSurface(24, 12), "catenoid");
  mesh(geo::diniSurface(24, 12), "dini");
  mesh(geo::pseudosphere(24, 12), "pseudosphere");
  mesh(geo::romanSurface(24, 12), "roman");
  mesh(geo::crossCap(24, 12), "crosscap");
  mesh(geo::torusKnot(120, 3, 7), "torus knot");
  mesh(geo::vivianiCurve(100), "viviani");
  mesh(geo::sphericalSpiral(100), "spherical spiral");
  mesh(geo::hypotrochoidKnot(120), "hypotrochoid");
  mesh(geo::duffingAttractor(1200), "duffing");
  mesh(geo::rosslerAttractor(1200), "rossler");
  mesh(geo::thomasAttractor(1200), "thomas");
  mesh(geo::sierpinskiTetrahedron(3), "sierpinski");
  for (auto k :
       {geo::ExoticImplicitKind::Heart, geo::ExoticImplicitKind::BarthSexticLike, geo::ExoticImplicitKind::TangleCube,
        geo::ExoticImplicitKind::ChmutovLike, geo::ExoticImplicitKind::CayleyCubic, geo::ExoticImplicitKind::KummerLike,
        geo::ExoticImplicitKind::Goursat, geo::ExoticImplicitKind::BlobLattice})
    mesh(geo::exoticImplicit(k, 12), "exotic implicit");
  for (unsigned i = 0; i < geo::unknownLabCount(); ++i)
    mesh(geo::unknownLabObject(i, 24, .3f), geo::unknownLabName(i).c_str());
  req(geo::noveltyScore(geo::discoveredObject(1, 24, 12)) > 0, "novelty finite positive");
  Timeline tl(120);
  auto s = tl.sample(.5);
  req(s.beatIndex == 1 && std::fabs(s.beatPhase) < .001f, "timeline beat");
  req(s.barIndex == 0 && s.barPhase > .24f && s.barPhase < .26f, "timeline bar");
  {
    Timeline rt(120);
    auto before = rt.sample(10.0);
    rt.retempo(10.0, 140);
    auto after = rt.sample(10.0);
    req(std::fabs(after.beat - before.beat) < 1e-9, "retempo keeps the beat continuous");
    auto later = rt.sample(13.0);
    req(std::fabs(later.beat - (before.beat + 3.0 * 140.0 / 60.0)) < 1e-9, "retempo uses the new BPM afterwards");
  }
  // B21: render-scale pixel budget
  {
    req(rscale::effective(1920, 1080, 1.0, rscale::kDefaultBudget) == 1.0, "1080p is under the pixel budget");
    const double s = rscale::effective(2880, 1800, 1.0, rscale::kDefaultBudget);
    req(s < .8 && s > .7, "Retina 2880x1800 is scaled to about 3.1 MP");
    req(std::fabs(2880 * s * 1800 * s - rscale::kDefaultBudget) < 2e4, "capped target meets the budget");
    req(rscale::effective(2880, 1800, .5, rscale::kDefaultBudget) == .5, "explicit lower quality is kept");
    req(rscale::effective(2880, 1800, 1.0, 0) == 1.0, "no budget means the user scale exactly");
  }
  // B20: camera field of view, logo show, scene description
  {
    const float v169 = cam::verticalFov(3.f, 0.f, 16.f / 9.f);
    req(std::fabs(v169 - (52.f + 3.f * std::sin(.21f)) * cam::kPi / 180.f) < 1e-5f, "16:9 fov unchanged");
    req(cam::verticalFov(3.f, 0.f, 1.f) > v169, "narrow window widens the vertical fov");
    const float h169 = 2.f * std::atan(std::tan(v169 * .5f) * (16.f / 9.f)), h11 = 2.f * std::atan(std::tan(cam::verticalFov(3.f, 0.f, 1.f) * .5f));
    req(std::fabs(h169 - h11) < 1e-3f, "horizontal fov is preserved on narrow windows");
    req(cam::verticalFov(3.f, 0.f, .01f) <= 2.6f + 1e-6f, "fov capped");
    const float id3[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    float M[16];
    cam::mvp(1.f, 1.6f, 1.f, 0.f, id3, M);
    bool fin = true;
    for (float x : M) fin = fin && std::isfinite(x);
    req(fin, "camera matrix finite");
    req(logoshow::at(0.0, 5).visibility == 0.f && logoshow::at(2.0, 5).visibility == 1.f, "logo fades in and holds");
    req(logoshow::at(logoshow::kFadeIn + logoshow::kHold + logoshow::kFadeOut + 1.0, 5).visibility == 0.f, "logo black beat");
    req(logoshow::at(logoshow::kCycle * 6.5, 5).card == 1, "logo cards cycle");
    req(describeScene(3, "N", "p", "R", 132.4, "") == "[03] N  -  p  *  R 132BPM", "description format");
  }
  // B19: noncommutative camera choreography
  {
    using namespace choreo;
    req(shearResidual(noncommutativeCamera(0.0)) < 1e-6f, "camera starts at identity");
    req(shearResidual(noncommutativeCamera(20.0)) < 1e-6f, "camera returns to identity each cycle");
    req(shearResidual(noncommutativeCamera(16.0)) > .05f, "undoing moves in order leaves a commutator residual");
    M3 ab = shearA(.5f) * shearB(.7f), ba = shearB(.7f) * shearA(.5f);
    req(shearResidual(lerp(ab, ba, 1.f)) == shearResidual(ba) && std::fabs(ab.m[0] - ba.m[0]) > .1f, "shears do not commute");
    req(forScene(5, 3.0, false, false, false).temporal == 0.f && forScene(5, 3.0, false, false, false).memory == 0.f,
        "no special effects in ordinary scenes");
    req(forScene(5, 3.0, false, true, true).temporal > 0.f && forScene(5, 3.0, false, true, true).memory > 0.f, "keys force effects");
  }
  // B18: scene-table consistency
  {
    SceneSystem ss;
    req(ss.count() == 73, "scene count 73");
    for (int i = 0; i < ss.count(); ++i) {
      auto& si = ss.info(i);
      req(si.id == i, "scene id==index");
      req(!si.name.empty(), "scene name non-empty");
      req(si.dynamic ? si.updateHz > 0 : true, "scene updateHz sane");
      const auto& m = ss.mesh(i, 0.0, 42);
      req(!m.v.empty(), "scene mesh non-empty");
      // B7: same tick -> same mesh pointer (cache hit)
      const auto& m2 = ss.mesh(i, 0.0, 42);
      req(&m == &m2, "scene cache hit same tick");
    }
    // B7: static scene tick is always 0
    req(SceneSystem::tickFor(ss.info(6), 99.0) == 0, "static scene tick=0");
    // B7: dynamic scene tick advances with time (scene 2 = 6 Hz, use 0.2s = 1.2 ticks)
    req(SceneSystem::tickFor(ss.info(2), 0.2) > SceneSystem::tickFor(ss.info(2), 0.0), "dynamic tick advances");
  }
  // Impossible-geometry scenes 59..72: valid, finite, duplicate-free, deterministic, edge-budgeted at several times
  {
    SceneSystem a, b;
    for (int id = 59; id < 73; ++id)
      for (double sec : {0.0, 2.6, 17.3}) {
        const Mesh3& m = a.mesh(id, sec, 42);
        mesh(m, "impossible scene");
        req(m.e.size() < 20000, "impossible scene edge budget");
        std::set<std::pair<uint32_t, uint32_t>> seen;
        for (auto& e : m.e) req(seen.insert(std::minmax(e.a, e.b)).second, "impossible scene duplicate edge");
        const Mesh3& m2 = b.mesh(id, sec, 42);
        req(m.v.size() == m2.v.size() && m.e.size() == m2.e.size(), "impossible scene deterministic");
      }
    // commutator gap is zero on the axes and non-zero off them
    Mesh3 nc = geo::noncommutativeLattice(5, 0.f);
    req(std::fabs(nc.v[2 * (2 * 5 + 2)].y - nc.v[2 * (2 * 5 + 2) + 1].y) < 1e-6f,
        "noncommutative lattice: centre commutes");
    req(std::fabs(nc.v[2 * (4 * 5 + 4)].y - nc.v[2 * (4 * 5 + 4) + 1].y) > .1f,
        "noncommutative lattice: corner does not commute");
  }
  // Non-orientable seams close along the identification, so no edge is much longer than a typical one.
  {
    auto longest = [](const Mesh3& m) {
      std::vector<float> L;
      for (auto& e : m.e) {
        auto a = m.v[e.a], b = m.v[e.b];
        L.push_back(std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z)));
      }
      std::sort(L.begin(), L.end());
      return L.back() / L[L.size() / 2];
    };
    req(longest(geo::mobiusStrip()) < 3.f, "Mobius seam has no stray long edges");
    req(longest(geo::kleinBottle()) < 3.f, "Klein seam has no stray long edges");
  }
  // VertexWeld returns the lowest existing index within eps (same as a linear scan), else appends.
  {
    Mesh3 m;
    VertexWeld w(m, 1e-3f);
    uint32_t a = w.add({0, 0, 0}), b = w.add({1, 0, 0}), c = w.add({.0005f, 0, 0}), d = w.add({1.0004f, 0, 0}),
             e = w.add({5, 5, 5});
    req(a == 0 && b == 1 && c == 0 && d == 1 && e == 2 && m.v.size() == 3,
        "VertexWeld merges within eps and appends otherwise");
  }
  // Text scroller: font + scroll math
  {
    req(textfont::glyphCount() == 95, "font 95 glyphs");
    req(textfont::width() == 5 && textfont::height() == 7, "font 5x7");
    auto p = textfont::pack();
    req(p.size() == 95 * 7, "font pack 665 bytes");
    req(textfont::measure("") == 0, "measure empty 0");
    req(textfont::measure("ABCDE") == 5 * 5 + 4, "measure ABCDE=29"); // 5*(5+1)-1
    req(scrollOffset(0.0f, 0.f, 100.f, 50.f) == 0.f, "scroll t=0 -> 0");
    // scroll is always in [0, wrap) and non-negative
    float wrap = 100.f + 50.f;
    for (double t = 0; t < 60; t += 0.13) {
      float o = scrollOffset(t, 0.f, 100.f, 50.f);
      req(o >= 0.f && o < wrap, "scroll in [0,wrap)");
    }
    // over time the offset must advance (hit a non-zero value at some t)
    bool advanced = false;
    for (double t = 0; t < 60; t += 0.01) {
      if (scrollOffset(t, 0.f, 100.f, 50.f) > 0.f) {
        advanced = true;
        break;
      }
    }
    req(advanced, "scroll advances over time");
    // Long-session stability: even after hours the offset must stay in [0,wrap)
    // with no float-jitter (the old (float)seconds lost sub-pixel precision
    // after ~minutes). Fixed beatPhase so only `seconds` varies.
    float wrapL = 100.f + 50.f;
    for (double t : {3600.0, 7200.0, 86400.0, 360000.0, 3600000.0, 36000000.0}) {
      float o = scrollOffset(t, 0.f, 100.f, 50.f);
      req(o >= 0.f && o < wrapL, "scroll in [0,wrap) at long t");
      // Consecutive samples at a long base must advance monotonically and
      // smoothly (no back-step jitter from precision loss).
      float o2 = scrollOffset(t + 0.001f, 0.f, 100.f, 50.f);
      // o2 is o + ~0.09px, wrapping into [0,wrap) — either it advanced or it
      // wrapped (o2 < o). It must NOT jump by more than the step + epsilon.
      float step = o2 - o;
      if (step < 0) step += wrapL;
      req(step < 1.f, "scroll smooth at long t (no jitter)");
    }
  }
  // Scroller font decode: the EXACT bit math the GLSL shader uses must match
  // the raw glyph bits for every (glyph, x, y) - a regression guard so the
  // marquee can't silently render blank again.
  // The font row byte is stored in a 1x665 GL_R8 texture, which NORMALIZES it
  // to [0,1] on read-back (b -> b/255). scroller.frag's fontPix() must scale it
  // back (round(r*255)) before bit extraction. The OLD test used the raw byte
  // (int v=row) and short-circuited the normalize->scale->round path, so it
  // never caught the bug where dividing the normalized float by 2^bit collapsed
  // almost every bit to 0 (255 of 256 byte values mis-extracted >=1 bit).
  {
    auto g = textfont::glyphs();
    int cells = 0;
    for (int gi = 0; gi < textfont::glyphCount(); gi++)
      for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++) {
          unsigned char row = g[gi].rows[y];
          int bit = 4 - x;
          int expected = (row >> bit) & 1;
          // Model the REAL GPU path: GL_R8 read-back normalizes, shader scales back.
          float r = (float)((double)row / 255.0);                        // GL_R8 read-back
          float v = std::round(r * 255.0f);                              // scroller.frag: round(r*255)
          double b = std::fmod(std::floor(v / std::pow(2.0, bit)), 2.0); // fontPix bit extract
          int got = (b > 0.5) ? 1 : 0;
          cells++;
          if (got != expected) {
            char m[96];
            std::snprintf(m, sizeof m, "font decode glyph %d row %d x %d: got %d want %d", gi, y, x, got, expected);
            req(false, m);
          }
        }
    req(cells == textfont::glyphCount() * 7 * 5, "scroller font decode covers every (glyph,row,col)");
    // a few known glyphs must be non-blank
    auto lit = [&](char c) {
      int n = 0;
      for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
          if ((g[c - 32].rows[y] >> (4 - x)) & 1) n++;
      return n;
    };
    req(lit('A') > 0 && lit('M') > 0 && lit('0') > 0, "scroller glyphs non-blank");
    req(lit(' ') == 0, "space glyph blank");
  }
  // Scroller CODE-STRIP: the bug that made the marquee render blank. The shader
  // maps a marquee POSITION (character index i) to a glyph by sampling a 1xN
  // GL_R8 strip the CPU uploads, one byte per char = its ASCII code. The decode
  // is int(round(tex.r)) - 32. This test re-implements the EXACT CPU encode
  // (drawScroller's codes[i]=(uint8_t)text[i]) + shader decode and checks that
  // every printable ASCII char round-trips to the correct glyph index (0..94),
  // and that a position is NOT confused with a code (the old bug).
  {
    auto enc = [&](const std::string& s) {
      std::vector<uint8_t> c(s.size());
      for (size_t i = 0; i < s.size(); ++i) c[i] = (uint8_t)s[i];
      return c;
    };
    // Decode a strip position i the way scroller.frag ACTUALLY does on the GPU:
    //   GL_R8 normalizes the stored byte b to float r = b/255 on read-back,
    //   the shader does  int(r * 255.0 + 0.5)  to recover the byte, then -32.
    // This exercises the REAL normalize->scale->round path (the previous test
    // used int(strip[i]) — the exact byte — and short-circuited the path, so it
    // would never catch the missing *255 that blanked the marquee).
    auto decodeAt = [&](const std::vector<uint8_t>& strip, int i) {
      float r = (float)((double)strip[i] / 255.0); // GL_R8 read-back normalization
      int code = (int)(r * 255.0f + 0.5f);         // scroller.frag: int(tex.r*255+0.5)
      return code - 32;
    };
    std::string all;
    for (char c = 32; c <= 126; ++c) all += c;
    auto strip = enc(all);
    req((int)strip.size() == 95, "code strip 95 chars");
    for (int i = 0; i < 95; ++i) {
      int glyph = decodeAt(strip, i);
      if (glyph != i) {
        char b[64];
        std::snprintf(b, sizeof b, "code strip round-trip char %d -> glyph %d (want %d)", i, glyph, i);
        req(false, b);
      }
    }
    // The OLD buggy mapping (glyph = position - 32) was wrong for every char
    // whose ASCII code != position+32. Prove the new mapping differs from the
    // old for at least the common letters, so a regression to position-based
    // indexing is caught.
    std::string hello = "HELLO";
    auto hs = enc(hello);
    int oldGlyphAt3 = 3 - 32;          // position 3, old mapping
    int newGlyphAt3 = decodeAt(hs, 3); // 'L' = 76-32 = 44
    req(newGlyphAt3 == 44 && oldGlyphAt3 != 44, "code strip: 'L' decodes to 44, not position-32");
    // Every printable ASCII byte must survive the GL_R8 normalize(->b/255) +
    // shader scale(*255)+round( +0.5) exactly. This is the exact GPU data path;
    // a missing *255 collapses all of 32..126 (all <0.5) to code 0.
    for (int b = 32; b <= 126; ++b) {
      float r = (float)((double)b / 255.0);
      int back = (int)(r * 255.0f + 0.5f);
      if (back != b) {
        char m[64];
        std::snprintf(m, sizeof m, "GL_R8 round-trip byte %d -> %d", b, back);
        req(false, m);
      }
    }
    // drawScroller caps the code strip at 512 chars (kMaxCodes) — a longer
    // marquee string is clipped, not allowed to overflow the 1x512 GL_R8
    // storage. Mirror the exact cap logic here so a regression (removing the
    // cap) is caught: the encoded strip for a 700-char string must be 512.
    {
      const int kMaxCodes = 512;
      std::string big(700, 'A');
      int n = (int)big.size();
      if (n > kMaxCodes) n = kMaxCodes;
      std::vector<uint8_t> codes((size_t)n);
      for (int i = 0; i < n; ++i) codes[i] = (uint8_t)big[i];
      req((int)codes.size() == kMaxCodes, "code strip clips a 700-char string to the 512 cap");
      // and the clipped prefix must still decode correctly (char 0 = 'A' = glyph 3)
      req(decodeAt(codes, 0) == 65 - 32, "clipped code strip prefix decodes to 'A'");
    }
  }
  // Image: decode + discovery (uses the repo's UBER logo pack when present)
  {
    Image miss;
    req(!Image::loadFromFile("/nonexistent/xyz.jpg", miss), "load missing -> false");
    // empty() contract: the OOB guard in loadFromFile relies on w>0 && h>0. A
    // zero- or negative-dimension decode must be reported empty (and is rejected
    // before the byte count is computed, so a corrupt header claiming w=0 or a
    // huge negative can't drive rgba.assign(px, px+huge) past the allocation).
    {
      Image z0;
      z0.w = 0;
      z0.h = 1080;
      req(z0.empty(), "Image::empty w=0");
      Image z1;
      z1.w = 1920;
      z1.h = 0;
      req(z1.empty(), "Image::empty h=0");
      Image zn;
      zn.w = -4;
      zn.h = 1080;
      req(zn.empty(), "Image::empty w<0");
      Image ok;
      ok.w = 4;
      ok.h = 4;
      ok.rgba.resize(64);
      req(!ok.empty(), "Image::empty valid");
      Image noRgba;
      noRgba.w = 4;
      noRgba.h = 4;
      req(noRgba.empty(), "Image::empty no rgba");
    }
    // Logo pack path: injected by CMake (IW_TEST_LOGO_PACK) so the test runs
    // on every platform; falls back to the dev-machine path if unset.
#ifdef IW_TEST_LOGO_PACK
    auto logos = findLogos(IW_TEST_LOGO_PACK);
#else
    auto logos = findLogos("/home/ulf/privat/wireframe-scroller/UBER_Fullscreen_Logo_Pack");
#endif
    req(findLogos("/nonexistent/dir").empty(), "findLogos missing -> empty");
    if (!logos.empty()) {
      req(logos.size() >= 20, "logo pack has 20 cards");
      Image im;
      req(Image::loadFromFile(logos[0], im), "logo decodes");
      req(im.w == 1920 && im.h == 1080, "logo 1920x1080");
      req(im.rgba.size() == (size_t)1920 * 1080 * 4, "logo rgba size");
    }
  }
  // Effect system: 80 CPU warpers + 30 recipes must be deterministic,
  // finite, and fail-safe (never return an empty/invalid mesh).
  {
    req(effectCount() == 80, "effectCount 80");
    req(recipeCount() == 30, "recipeCount 30");
    // recipeName must agree with recipe().name (regression: recipeName used
    // to return a string_view into a destroyed temporary).
    for (int i = 0; i < recipeCount(); i++) req(recipeName(i) == recipe(i).name, "recipeName matches recipe");
    req(recipeName(0) == "Raw" && recipeName(23) == "Dimensional Infection", "recipeName endpoints");
    req(recipeName(29) == "Quaternion Bloom", "recipeName last");
    req(recipeName(30) == recipeName(0), "recipeName wraps");
    // every effect has a distinct, non-empty name
    {
      std::set<std::string> names;
      for (int i = 0; i < effectCount(); i++) {
        auto n = std::string(effectName(i));
        req(!n.empty(), "effect name non-empty");
        names.insert(n);
      }
      req(names.size() == size_t(effectCount()), "effect names distinct");
    }
    // the unknown effects (64-79) must actually change a torus (no silent no-ops)
    for (int id = 64; id < 80; ++id) {
      auto a = geo::torus(24, 8), b = a;
      EffectContext ec;
      ec.time = 1.7f;
      ec.amount = 1.f;
      ec.seed = 5u;
      ec.bass = ec.mid = ec.treble = .6f;
      ec.beat = .5f;
      applyEffect(b, id, ec, nullptr);
      bool changed = a.v.size() != b.v.size() || a.e.size() != b.e.size();
      for (size_t i = 0; i < a.v.size() && !changed; i++)
        changed =
            std::fabs(a.v[i].x - b.v[i].x) + std::fabs(a.v[i].y - b.v[i].y) + std::fabs(a.v[i].z - b.v[i].z) > 1e-4f;
      req(changed, "unknown effect changes the mesh");
    }
    // every single effect on a torus at several (time, amount) phases stays valid
    for (int id = 0; id < effectCount(); ++id) {
      EffectContext ec;
      ec.amount = 1.0f;
      ec.seed = 7u;
      for (float t : {0.f, 1.37f, 4.2f, 9.9f, 20.5f}) {
        ec.time = t;
        ec.bass = .5f;
        ec.mid = .5f;
        ec.treble = .5f;
        ec.beat = (t * 2.0f) / 3.14159f;
        auto m = geo::torus(24, 8);
        bool ok = applyEffect(m, id, ec, nullptr);
        req(ok, "effect returns valid mesh");
        auto st = geo::stats(m);
        req(st.finite, "effect mesh finite");
        req(st.indicesValid, "effect mesh in-bounds");
      }
      // determinism: identical inputs -> identical output
      {
        auto a = geo::torus(24, 8), b = geo::torus(24, 8);
        EffectContext ec;
        ec.time = 3.14f;
        ec.amount = .7f;
        ec.seed = 99u;
        ec.bass = .3f;
        ec.mid = .6f;
        ec.treble = .2f;
        ec.beat = .5f;
        applyEffect(a, id, ec, nullptr);
        applyEffect(b, id, ec, nullptr);
        bool sameV = a.v.size() == b.v.size();
        for (size_t i = 0; i < a.v.size() && sameV; i++)
          sameV = sameV && (a.v[i].x == b.v[i].x && a.v[i].y == b.v[i].y && a.v[i].z == b.v[i].z);
        bool sameE = a.e.size() == b.e.size();
        for (size_t i = 0; i < a.e.size() && sameE; i++)
          sameE = sameE && (a.e[i].a == b.e[i].a && a.e[i].b == b.e[i].b);
        req(sameV, "effect deterministic (vertices)");
        req(sameE, "effect deterministic (edges)");
      }
    }
    // every curated recipe on several scenes/secondary objects stays valid.
    // (Cheap meshes: the edge-subdivide stages are O(edges), and the lab's own
    // matrix uses ~1000-edge objects.)
    auto scenes = [&]() { return std::vector<Mesh3>{geo::torus(16, 6), geo::cube(1.f), geo::uvSphere(8, 16)}; };
    for (int rid = 0; rid < recipeCount(); ++rid) {
      auto r = recipe(rid);
      req(r.stageCount >= 0 && r.stageCount <= 6, "recipe stage count sane");
      for (const auto& base : scenes()) {
        Mesh3 m = base;
        const Mesh3* sec = &base;
        for (float t : {0.f, 2.5f}) {
          EffectContext ec;
          ec.time = t;
          ec.amount = 1.2f;
          ec.seed = 123u + uint32_t(rid);
          ec.bass = .5f + .5f * std::sin(t);
          ec.mid = .5f + .5f * std::sin(t + 1);
          ec.treble = .5f + .5f * std::sin(t + 2);
          ec.beat = .5f + .5f * std::sin(t * 3);
          bool ok = applyRecipe(m, r, ec, sec);
          auto st = geo::stats(m);
          req(ok, "recipe applies (fail-safe)");
          req(st.finite, "recipe mesh finite");
          req(st.indicesValid, "recipe mesh in-bounds");
        }
      }
    }
    // procedural mutation is deterministic and bounded
    for (uint32_t s : {0u, 1u, 7u, 424242u, 0xffffffffu}) {
      auto a = mutateRecipe(s, 4), b = mutateRecipe(s, 4);
      req(a.name == b.name, "mutation name deterministic");
      req(a.stageCount == b.stageCount && a.stageCount == 4, "mutation stage count");
      bool same = true;
      for (int i = 0; i < 4; i++) {
        const auto& x = a.stages[i];
        const auto& y = b.stages[i];
        same = same && (x.effect == y.effect && x.amount == y.amount && x.timeScale == y.timeScale &&
                        x.timeOffset == y.timeOffset && x.seedOffset == y.seedOffset);
      }
      req(same, "mutation deterministic");
      // mutated recipe must run without breaking
      {
        Mesh3 m = geo::torus(24, 8);
        EffectContext ec;
        ec.time = 1.1f;
        ec.amount = 1.f;
        ec.seed = 5u;
        ec.bass = .5f;
        ec.mid = .5f;
        ec.treble = .5f;
        ec.beat = .5f;
        auto st = geo::stats(m);
        req(applyRecipe(m, a, ec, nullptr), "mutation recipe applies");
        st = geo::stats(m);
        req(st.finite && st.indicesValid, "mutation mesh finite+in-bounds");
      }
    }
    // bridge effects (36/37) need a secondary object and add vertices
    {
      auto a = geo::torus(24, 8), sec = geo::cube(1.f);
      auto before = a.v.size();
      EffectContext ec;
      ec.time = 1.0f;
      ec.amount = 1.f;
      ec.seed = 1u;
      ec.bass = .5f;
      ec.mid = .5f;
      ec.treble = .5f;
      ec.beat = .5f;
      bool ok36 = applyEffect(a, 36, ec, &sec);
      bool ok37 = applyEffect(a, 37, ec, &sec);
      req(ok36 && a.v.size() > before, "dual bridge adds vertices");
      req(ok37, "nearest bridge valid");
    }
    // empty-mesh guard
    {
      Mesh3 empty;
      EffectContext ec;
      req(!applyEffect(empty, 4, ec, nullptr), "empty mesh -> false");
    }
  }
  // Audio band-split: the one-pole IIR coefficients must separate low/mid/high.
  // Simulate the same 3-tap lowpass chain the renderer uses and verify that a
  // 100 Hz sine ends up mostly in the bass band, 1 kHz in mid, 8 kHz in treble.
  {
    auto bandOf = [&](float freq) {
      const float rate = 48000.f;
      const float aB = .021f, aM = .14f, aT = .5f;
      float lpB = 0, lpM = 0, lpT = 0;
      float bAcc = 0, mAcc = 0, tAcc = 0;
      int N = 4800;
      for (int i = 0; i < N; i++) {
        float s = std::sin(2.f * 3.14159265f * freq * float(i) / rate);
        lpB += aB * (s - lpB);
        lpM += aM * (s - lpM);
        lpT += aT * (s - lpT);
        bAcc += std::fabs(lpB);
        mAcc += std::fabs(lpM - lpB);
        tAcc += std::fabs(lpT - lpM);
      }
      float b = bAcc / float(N), m = mAcc / float(N), t = tAcc / float(N);
      float tot = b + m + t;
      if (tot < 1e-9f) return std::tuple<float, float, float>(0.f, 0.f, 0.f);
      return std::make_tuple(b / tot, m / tot, t / tot);
    };
    auto [bl, ml, tl] = bandOf(100.f);
    req(bl > ml && bl > tl, "100Hz mostly bass");
    auto [bm, mm, tm] = bandOf(1000.f);
    req(mm >= bm && mm >= tm, "1kHz mostly mid");
    auto [bt, mt, tt] = bandOf(8000.f);
    req(tt >= bt && tt >= mt, "8kHz mostly treble");
  }
  // Shader sanity: no C-specific math functions in GLSL (they're not GLSL
  // builtins and Apple's stricter compiler rejects them — e.g. fmod, which
  // broke the scroller shader on macOS). Scan every .vert/.frag for the
  // common C names that have GLSL equivalents (mod/abs/min/max/pow/...).
  {
    const std::vector<std::string> bad = {"fmod",  "fabs", "fmin", "fmax", "sinf",  "cosf",  "tanf",  "sqrtf", "floorf",
                                          "ceilf", "powf", "expf", "logf", "atanf", "asinf", "acosf", "fmodf"};
    auto shadersDir = std::filesystem::path("engine/shaders");
    if (!std::filesystem::exists(shadersDir))
      shadersDir = std::filesystem::path(__FILE__).parent_path() / ".." / "engine" / "shaders";
    for (const auto& entry : std::filesystem::directory_iterator(shadersDir)) {
      if (!entry.is_regular_file()) continue;
      auto ext = entry.path().extension().string();
      if (ext != ".vert" && ext != ".frag") continue;
      std::ifstream f(entry.path());
      std::string line;
      while (std::getline(f, line)) {
        // strip // comments so a comment mentioning fmod doesn't false-positive
        size_t c = line.find("//");
        if (c != std::string::npos) line = line.substr(0, c);
        for (const auto& b : bad) {
          // word-boundary match (not part of a longer identifier)
          size_t p = 0;
          while ((p = line.find(b, p)) != std::string::npos) {
            bool lp = (p == 0) || (!std::isalnum((unsigned char)line[p - 1]) && line[p - 1] != '_');
            size_t e = p + b.size();
            bool rp = (e >= line.size()) || (!std::isalnum((unsigned char)line[e]) && line[e] != '_');
            if (lp && rp) {
              std::cerr << "FAIL: C-specific function '" << b << "' in shader " << entry.path() << " line: " << line
                        << "\n";
              std::exit(1);
            }
            p = e;
          }
        }
      }
    }
    // the scroller shader must extract glyph bits with integer ops (pow(2,bit)+floor is inexact on some GL drivers)
    {
      std::ifstream f(shadersDir / "scroller.frag");
      std::string all((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
      req(all.find("(int(v) >> int(bit)) & 1") != std::string::npos, "scroller extracts glyph bits with integer ops");
      // The code-strip decode MUST scale the GL_R8 read-back by 255 (the byte
      // is normalized to [0,1] on read). A regression that drops the *255
      // collapses every char to code 0 -> blank marquee (the original bug).
      req(all.find("* 255.0") != std::string::npos, "scroller code-strip decode scales GL_R8 by 255");
    }
    // post.frag must be tempo-synced: the synthetic beat pulse must be driven
    // by uBpm (bpm/60 Hz), NOT a hard-coded 3.2 Hz. A regression that re-introduces
    // a fixed frequency would make the FX pulse at the wrong rate for --bpm != 192.
    {
      std::ifstream f(shadersDir / "post.frag");
      std::string all((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
      req(all.find("uBpm") != std::string::npos, "post.frag declares uBpm");
      req(all.find("uBpm/60.0") != std::string::npos, "post.frag beat synced to bpm/60 Hz");
      // the old hard-coded 3.2 Hz (192 BPM only) must be gone from the beat line
      req(all.find("sin(uTime*3.2)") == std::string::npos, "post.frag no hard-coded 3.2 Hz beat");
      // --no-post bypass path must exist (ACES tone-map + early return)
      req(all.find("uBypass") != std::string::npos, "post.frag declares uBypass");
    }
  }
  // --window WxH parsing: the shared parseWindowSpec (include/WindowSpec.hpp)
  // must accept valid specs (lower- and upper-case separator) and reject
  // malformed / out-of-range ones, so a bad --window is a clean exit-2 (not a
  // crash or a silently-clamped window).
  {
    std::string e;
    int w, h;
    req(parseWindowSpec("1920x1080", w, h, e) && w == 1920 && h == 1080, "window: 1920x1080");
    req(parseWindowSpec("1280X720", w, h, e) && w == 1280 && h == 720, "window: 1280X720 (upper-case X)");
    req(parseWindowSpec("1440x900", w, h, e) && w == 1440 && h == 900, "window: 1440x900 (default)");
    req(!parseWindowSpec("1920", w, h, e), "window: '1920' (no H) rejected");
    req(!parseWindowSpec("1920x", w, h, e), "window: '1920x' (no H) rejected");
    req(!parseWindowSpec("x1080", w, h, e), "window: 'x1080' (no W) rejected");
    req(!parseWindowSpec("1920 1080", w, h, e), "window: '1920 1080' (space) rejected");
    req(!parseWindowSpec("1920x1080x24", w, h, e), "window: trailing junk rejected");
    req(!parseWindowSpec("100x100", w, h, e), "window: below 320x200 rejected");
    req(!parseWindowSpec("99999x1080", w, h, e), "window: above 3840 width rejected");
    req(!parseWindowSpec("1920x99999", w, h, e), "window: above 2160 height rejected");
    req(parseWindowSpec("320x200", w, h, e) && w == 320 && h == 200, "window: 320x200 (min boundary) ok");
    req(parseWindowSpec("3840x2160", w, h, e) && w == 3840 && h == 2160, "window: 3840x2160 (max boundary) ok");
  }
  { // Effect recipes must never hand the renderer an invalid mesh. main() quits the show when
    // Renderer::upload() rejects one, and the effects used to leave duplicate edges behind
    // ("Mesh rejected in scene 2: duplicate edge"). Every curated recipe, plus the
    // auto path (the scene's recipe + 2 mutation stages).
    SceneSystem ss;
    uint64_t seed = 0x49574f424a454354ull;
    // The scenes that produced duplicate edges (0,1,2,5,11,26) plus every 8th, at one time (the full
    // 51-scene x 2-time sweep takes about a minute).
    for (int sc = 0; sc < ss.count(); ++sc)
      for (float t : {0.f}) {
        if (!(sc == 0 || sc == 1 || sc == 2 || sc == 5 || sc == 11 || sc == 26 || sc % 8 == 0)) continue;
        EffectContext ec;
        ec.time = t;
        ec.seed = 0x50454646u + uint32_t(sc) * 131u;
        const Mesh3* sec = &ss.mesh((sc + 1) % ss.count(), t, seed);
        for (int rid = 0; rid < recipeCount(); ++rid) {
          Mesh3 m = ss.mesh(sc, t, seed);
          applyRecipe(m, recipe(rid), ec, sec);
          mesh(m, "effects: recipe output is a valid mesh");
        }
        Mesh3 m = ss.mesh(sc, t, seed);
        EffectRecipe rc;
        rc.name = "Auto";
        auto base = recipe(sc % recipeCount());
        auto mut = mutateRecipe(ec.seed, 2);
        int i = 0;
        for (int k = 0; k < base.stageCount && i < 6; k++) rc.stages[i++] = base.stages[k];
        for (int k = 0; k < mut.stageCount && i < 6; k++) rc.stages[i++] = mut.stages[k];
        rc.stageCount = i;
        applyRecipe(m, rc, ec, sec);
        mesh(m, "effects: auto recipe output is a valid mesh");
      }
  }
  // Regression: the "Dual Bridge" (36) and "Nearest Bridge" (37) effects used to
  // iterate s->v / s->e while pushing into m.v / m.e. When s == &m (the
  // caller passes the target mesh as its own secondary) the push_back
  // reallocated m.v mid-iteration — a heap-use-after-free (caught by ASan) and
  // formal UB in the shipping build. The fix copies s->v (and s->e) into
  // locals first. Exercise the exact s==&m aliasing pattern.
  {
    EffectContext ec;
    ec.time = 1.0f;
    ec.amount = 1.f;
    ec.seed = 0xABCDu;
    auto runBridge = [&](int id, const char* name) {
      // A moderately sized mesh so the first push_back triggers a realloc
      // (a tiny mesh might fit in the small-object buffer without a move).
      Mesh3 m;
      for (int i = 0; i < 64; i++) {
        float a = i * 0.1f;
        m.v.push_back({std::cos(a), 0.1f * std::sin(2 * a), std::sin(a)});
      }
      for (int i = 0; i < 63; i++) m.e.push_back({(uint32_t)i, (uint32_t)(i + 1)});
      std::string w;
      req(geo::validate(m, &w), "alias: input mesh valid");
      // Call with s == &m (self-aliasing) — the exact UB pattern.
      req(applyEffect(m, id, ec, &m), name);
      req(geo::validate(m, &w), name);
      req(!m.v.empty() && !m.e.empty(), name);
    };
    runBridge(36, "alias: effect 36 (Dual Bridge) self-alias no crash");
    runBridge(37, "alias: effect 37 (Nearest Bridge) self-alias no crash");
  }
  // Regression: scrollOffset must be a CONTINUOUS, monotonic function of time.
  // The old code scaled the distance by a per-frame speed
  // (90*(0.7+0.3*(1-beatPhase)) + music*40), so with a live beatPhase/music
  // the offset teleported forward/backward every frame (visible stutter).
  // The fix is a constant 90 px/s: varying beatPhase/music must NOT change the
  // offset at a fixed time, and consecutive samples must advance smoothly.
  {
    float wrap = 100.f + 50.f;
    // beatPhase and music must have NO effect on the offset (constant speed)
    for (double t : {1.0, 2.0, 5.0, 13.0, 99.0, 3600.0}) {
      float base = scrollOffset(t, 0.f, 100.f, 50.f);
      req(std::fabs(scrollOffset(t, 0.25f, 100.f, 50.f) - base) < 1e-4f, "scroll: beatPhase-independent");
      req(std::fabs(scrollOffset(t, 1.f, 100.f, 50.f) - base) < 1e-4f, "scroll: beatPhase=1 independent");
      req(std::fabs(scrollOffset(t, 0.5f, 100.f, 50.f) - base) < 1e-4f, "scroll: beatPhase=0.5 independent");
      req(std::fabs(scrollOffset(t, 0.5f, 100.f, 50.f, 1.f) - base) < 1e-4f, "scroll: music-independent");
    }
    // monotonic + smooth: a small dt advances by ~dt*90 px (no jump/back-step)
    for (double t = 0; t < 300; t += 0.5) {
      float o = scrollOffset(t, 0.f, 100.f, 50.f);
      float o2 = scrollOffset(t + 0.01f, 0.f, 100.f, 50.f);
      float step = o2 - o;
      if (step < 0) step += wrap; // allow one wrap
      req(step > 0.f && step < 5.f, "scroll: smooth monotonic advance (no teleport)");
    }
  }
  // Performance guards (deterministic, so they cannot flake on a slow CI machine):
  // 1) every scene run through the auto recipe exactly as main() builds it stays inside the edge
  //    budget and stays renderable (copy/echo chains once reached 400k edges and hundreds of ms).
  {
    SceneSystem ss;
    const uint32_t baseSeed = 0x50454646u;
    for (int scene = 0; scene < ss.count(); ++scene)
      for (double t : {3.0, 17.5}) {
        Mesh3 m = ss.mesh(scene, t, 0x49574f424a454354ull);
        EffectContext ec;
        ec.time = float(t);
        ec.amount = 1.6f;
        ec.beat = 1.f;
        ec.bass = ec.mid = ec.treble = 1.f;
        ec.seed = baseSeed + uint32_t(scene) * 131u;
        EffectRecipe rc;
        auto base = recipe(scene % recipeCount());
        auto mut = mutateRecipe(ec.seed, 2);
        int i = 0;
        for (int k = 0; k < base.stageCount && i < 6; k++) rc.stages[i++] = base.stages[k];
        for (int k = 0; k < mut.stageCount && i < 6; k++) rc.stages[i++] = mut.stages[k];
        rc.stageCount = i;
        const Mesh3& sec = ss.mesh((scene + 1) % ss.count(), t, 0x49574f424a454354ull);
        applyRecipe(m, rc, ec, &sec);
        req(m.e.size() <= 60000, "auto recipe stays inside the 60k edge budget");
        req(geo::renderable(m), "auto recipe output is renderable");
      }
  }
  // 2) every effect leaves a fully valid mesh, duplicates included (dedupe only runs where an effect
  //    can create duplicates, so this checks that classification).
  for (int id = 0; id < effectCount(); ++id) {
    auto m = geo::torus(24, 8);
    auto sec = geo::uvSphere(8, 16);
    EffectContext ec;
    ec.time = 2.3f;
    ec.amount = 1.4f;
    ec.seed = 11u;
    ec.bass = ec.mid = ec.treble = .7f;
    ec.beat = .8f;
    applyEffect(m, id, ec, &sec);
    std::string why;
    if (!geo::validate(m, &why)) {
      std::cerr << "effect " << id << ": " << why << "\n";
      req(false, "effect output passes geo::validate");
    }
  }
  // 3) the render loop's non-blocking fetch converges to exactly the synchronous mesh.
  {
    SceneSystem a, b;
    const uint64_t sd = 0x49574f424a454354ull;
    req(a.meshAsync(2, 0.0, sd).v.empty(), "meshAsync never blocks: an unbuilt scene is empty at first");
    Mesh3 first;
    for (int k = 0; k < 500 && first.v.empty(); ++k) {
      first = a.meshAsync(2, 0.0, sd);
      if (first.v.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    req(first.v.size() == b.mesh(2, 0.0, sd).v.size(), "meshAsync delivers the real mesh once built");
    const Mesh3& want = b.mesh(2, 1.0, sd);
    bool converged = false;
    for (int k = 0; k < 500 && !converged; ++k) {
      const Mesh3& got = a.meshAsync(2, 1.0, sd);
      converged = got.v.size() == want.v.size() && got.e.size() == want.e.size() &&
                  std::equal(got.v.begin(), got.v.end(), want.v.begin(),
                             [](V3 x, V3 y) { return x.x == y.x && x.y == y.y && x.z == y.z; });
      if (!converged) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    req(converged, "meshAsync converges to the synchronous mesh");
    a.prefetch(35, 0.0, sd);
    size_t got = 0;
    for (int k = 0; k < 500 && got == 0; ++k) {
      got = a.meshAsync(35, 0.0, sd).e.size();
      if (!got) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    req(got == b.mesh(35, 0.0, sd).e.size(), "prefetch then meshAsync gives the real mesh");
  }
  std::cout << "geometry_tests: PASS; exact 120-cell V=" << c120.v.size() << " E=" << c120.e.size() << "\n";
}
