#include "Effects.hpp"
#include <set>
#include <algorithm>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <limits>

namespace {
constexpr float PI = 3.14159265358979323846f;

int wrap(int x, int n) { x %= n; return x < 0 ? x + n : x; }
float clampf(float x, float a, float b) { return std::max(a, std::min(b, x)); }
uint32_t hash32(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x;
}
float rnd(uint32_t x) { return float(hash32(x) & 0xffffff) / float(0xffffff) * 2.f - 1.f; }

// V3 helpers (no operator overloads -> no ambiguity with other math).
float vdot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float vlen(V3 a) { return std::sqrt(vdot(a, a)); }
V3 vadd(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 vsub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 vmul(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
V3 vnorm(V3 a) { float q = vlen(a); return q > 1e-12f ? vmul(a, 1.f / q) : V3{0, 0, 0}; }
V3 vcross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
float hyp(V3 a) { return std::hypot(a.x, a.z); }

V3 center(const Mesh3& m) { V3 c{}; for (auto p : m.v) c = vadd(c, p); return m.v.empty() ? c : vmul(c, 1.f / float(m.v.size())); }
void mapv(Mesh3& m, const std::function<V3(V3, size_t)>& f) { for (size_t i = 0; i < m.v.size(); ++i) m.v[i] = f(m.v[i], i); }
void cull(Mesh3& m, const std::function<bool(V3, V3, size_t)>& f) {
  std::vector<Edge> out; out.reserve(m.e.size());
  for (size_t i = 0; i < m.e.size(); ++i) { const Edge& ed = m.e[i]; if (ed.a < m.v.size() && ed.b < m.v.size() && f(m.v[ed.a], m.v[ed.b], i)) out.push_back(ed); }
  m.e = std::move(out);
}
void copyInto(Mesh3& d, const Mesh3& s, V3 o, float sc, float a) {
  unsigned base = (unsigned)d.v.size(); float C = cosf(a), S = sinf(a);
  for (auto p : s.v) d.v.push_back({(p.x * C - p.z * S) * sc + o.x, p.y * sc + o.y, (p.x * S + p.z * C) * sc + o.z});
  for (auto i : s.e) d.e.push_back({base + i.a, base + i.b});
}
void subdiv(Mesh3& m, int n, float j, uint32_t seed) {
  n = std::clamp(n, 1, 8);
  // Cap edge growth: if the subdivision would produce >50k edges, skip it
  // (the mesh is already dense enough; heavy recipes on large meshes would
  // otherwise produce 100k+ edges per stage, killing frame time).
  if (m.e.size() * (size_t)n > 50000) return;
  Mesh3 o;
  for (const Edge& ed : m.e) {
    V3 a = m.v[ed.a], b = m.v[ed.b], dv = vsub(b, a), side = vnorm(vcross(dv, V3{.31f, .73f, .19f}));
    unsigned prev = (unsigned)o.v.size(); o.v.push_back(a);
    for (int k = 1; k <= n; k++) { float u = float(k) / n; V3 p = vadd(a, vmul(dv, u)); if (k < n) p = vadd(p, vmul(side, j * rnd(seed + (uint32_t)(o.e.size() * 17 + k)))); unsigned cur = (unsigned)o.v.size(); o.v.push_back(p); o.e.push_back({prev, cur}); prev = cur; }
  }
  m = std::move(o);
}

// Drops out-of-range, self and zero-length edges, and duplicate edges (the constellation/bridge
// effects re-add edges between already-connected vertices). Duplicates must go: the renderer's
// geo::validate() rejects them, and main() used to quit the whole show on the first rejected mesh;
// they would also be drawn twice and double the brightness of an additively blended line.
void sanitize(Mesh3& m) {
  std::vector<Edge> out; out.reserve(m.e.size());
  std::set<std::pair<uint32_t, uint32_t>> seen;
  for (const Edge& ed : m.e) {
    if (ed.a >= m.v.size() || ed.b >= m.v.size() || ed.a == ed.b) continue;
    if (vlen(vsub(m.v[ed.a], m.v[ed.b])) <= 1e-5f) continue;
    if (!seen.insert(std::minmax(ed.a, ed.b)).second) continue;
    out.push_back(ed);
  }
  m.e = std::move(out);
}
// The lab's fail-safe gate: finite, in-bounds, no degenerate/self edges (duplicates are removed by
// sanitize() before this runs).
bool meshOK(const Mesh3& m) {
  for (const auto& p : m.v) if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return false;
  for (const Edge& ed : m.e) if (ed.a >= m.v.size() || ed.b >= m.v.size() || ed.a == ed.b) return false;
  return true;
}
}  // namespace

std::string_view effectName(int id) {
  static const char* n[] = {"None", "Twist", "Bend", "Taper", "Pulse", "Ripple", "Noise Warp", "Quantize", "Mirror Fold", "Sphericalize", "Cubify", "Inversion", "Vortex", "Shear Wave", "Radial Wave", "Axis Permute", "Mobius Phase", "Kaleidoscope", "Polar Quantize", "Breathing Shell", "Pinch", "Explode", "Implode", "Shatter", "Edge Pulse", "Scan Slice", "Venetian Slice", "Checker Cull", "Radial Cull", "Depth Cull", "Phase Cull", "Echo Copies", "Orbit Copies", "Recursive Copies", "Trail Copies", "Time Shear", "Dual Bridge", "Nearest Bridge", "Centroid Spokes", "Constellation", "Edge Subdivide", "Edge Fracture", "Edge Braid", "Edge Lightning", "Gravity Lens", "Black Hole Ring", "Wormhole Pair", "Event Horizon", "Caustic Fold", "Projective Singularity", "Hyper Perspective", "Fisheye 3D", "Stereo Split", "Chromatic Geometry", "Audio Radial", "Audio Twist", "Audio Bands", "Beat Shatter", "Bass Breath", "Treble Noise", "Spectral Rings", "Flow Advection", "Curl Warp", "Domain Warp"};
  return n[wrap(id, effectCount())];
}
int effectCount() { return 64; }

bool applyEffect(Mesh3& m, int iid, const EffectContext& c, const Mesh3* s) {
  if (m.v.empty()) return false;
  Mesh3 original = m;
  int id = wrap(iid, effectCount());
  float A = clampf(c.amount, -4, 4), t = c.time;
  V3 C = center(m);
  auto q = [&](V3 p) { return vsub(p, C); };
  switch (id) {
  case 0: break;
  case 1: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float a = A * .7f * x.y + .2f * sinf(t), co = cosf(a), si = sinf(a); return vadd(C, V3{x.x * co - x.z * si, x.y, x.x * si + x.z * co}); }); break;
  case 2: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float a = A * .22f * x.x, co = cosf(a), si = sinf(a); return vadd(C, V3{x.x, x.y * co - x.z * si, x.y * si + x.z * co}); }); break;
  case 3: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float z = std::max(.05f, 1 + A * .12f * x.y); x.x *= z; x.z *= z; return vadd(C, x); }); break;
  case 4: mapv(m, [&](V3 p, size_t) { return vadd(C, vmul(q(p), 1 + A * .18f * sinf(t * 2))); }); break;
  case 5: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float r = vlen(x); return vadd(p, vmul(vnorm(x), A * .16f * sinf(r * 5 - t * 3))); }); break;
  case 6: mapv(m, [&](V3 p, size_t i) { return vadd(p, V3{A * .12f * rnd(c.seed + i * 3), A * .12f * rnd(c.seed + i * 3 + 1), A * .12f * rnd(c.seed + i * 3 + 2)}); }); break;
  case 7: mapv(m, [&](V3 p, size_t) { float z = std::max(.03f, .25f / std::max(.1f, fabsf(A))); return V3{std::isfinite(p.x) ? std::round(p.x / z) * z : p.x, std::isfinite(p.y) ? std::round(p.y / z) * z : p.y, std::isfinite(p.z) ? std::round(p.z / z) * z : p.z}; }); break;
  case 8: mapv(m, [&](V3 p, size_t) { V3 x = q(p); x.x = fabsf(x.x); if (fabsf(A) > 1) x.z = fabsf(x.z); return vadd(C, x); }); break;
  case 9: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float u = clampf(fabsf(A), 0, 1); return vadd(C, vadd(vmul(x, 1 - u), vmul(vnorm(x), 1.7f * u))); }); break;
  case 10: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float k = std::max({fabsf(x.x), fabsf(x.y), fabsf(x.z), 1e-5f}); return vadd(C, vmul(x, 1.7f / k)); }); break;
  case 11: mapv(m, [&](V3 p, size_t) { V3 x = q(p); return vadd(C, vmul(x, (1.8f + .2f * A) / std::max(.08f, vdot(x, x)))); }); break;
  case 12: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float r = hyp(x), a = A * 1.4f / (.35f + r) + t * .2f, co = cosf(a), si = sinf(a); return vadd(C, V3{x.x * co - x.z * si, x.y, x.x * si + x.z * co}); }); break;
  case 13: mapv(m, [&](V3 p, size_t) { p.x += A * .3f * sinf(p.y * 2.7f + t); p.z += A * .2f * cosf(p.x * 2.1f - t); return p; }); break;
  case 14: mapv(m, [&](V3 p, size_t) { V3 x = q(p); return vadd(C, vmul(x, 1 + A * .12f * sinf(vlen(x) * 6 - t * 2))); }); break;
  case 15: mapv(m, [&](V3 p, size_t) { return A >= 0 ? V3{p.y, p.z, p.x} : V3{p.z, p.x, p.y}; }); break;
  case 16: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float a = atan2f(x.z, x.x) * .5f * A, co = cosf(a), si = sinf(a); return vadd(C, V3{x.x, x.y * co - x.z * si, x.y * si + x.z * co}); }); break;
  case 17: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float r = hyp(x), a = atan2f(x.z, x.x), sg = PI / 3; a = fabsf(fmodf(a + PI, sg) - sg * .5f); return vadd(C, V3{r * cosf(a), x.y, r * sinf(a)}); }); break;
  case 18: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float r = hyp(x), a = atan2f(x.z, x.x), sg = PI / 8; a = std::round(a / sg) * sg; return vadd(C, V3{r * cosf(a), x.y, r * sinf(a)}); }); break;
  case 19: mapv(m, [&](V3 p, size_t) { V3 x = q(p); return vadd(C, vmul(x, 1 + A * .2f * sinf(t * 1.7f + atan2f(x.z, x.x) * 3))); }); break;
  case 20: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float z = 1 - A * .55f * expf(-x.y * x.y); x.x *= z; x.z *= z; return vadd(C, x); }); break;
  case 21: mapv(m, [&](V3 p, size_t i) { return vadd(p, vmul(vnorm(q(p)), A * .45f * (.5f + .5f * rnd(c.seed + (uint32_t)i)))); }); break;
  case 22: mapv(m, [&](V3 p, size_t) { return vadd(C, vmul(q(p), std::max(.02f, 1 - .35f * fabsf(A)))); }); break;
  case 23: mapv(m, [&](V3 p, size_t i) { uint32_t k = c.seed + (uint32_t)i + (uint32_t)floorf(t * 3) * 101u; return vadd(p, V3{A * .22f * rnd(k), A * .22f * rnd(k + 7), A * .22f * rnd(k + 19)}); }); break;
  case 24: mapv(m, [&](V3 p, size_t i) { return vadd(C, vmul(q(p), 1 + A * .14f * sinf(t * 4 + float(i) * .17f))); }); break;
  case 25: cull(m, [&](V3 a, V3 b, size_t) { float y = (sinf(t) * .5f + .5f) * 4 - 2; return fabsf((a.y + b.y) * .5f - y) < .45f; }); break;
  case 26: cull(m, [&](V3 a, V3 b, size_t) { return fmodf(fabsf((a.y + b.y) * 2 + t), 2) < 1; }); break;
  case 27: cull(m, [&](V3 a, V3 b, size_t) { V3 p = vmul(vadd(a, b), .5f); return (((int)floorf(p.x * 2) + (int)floorf(p.y * 2) + (int)floorf(p.z * 2)) & 1) != 0; }); break;
  case 28: cull(m, [&](V3 a, V3 b, size_t) { return fmodf(vlen(q(vmul(vadd(a, b), .5f))) * 3 + t, 2) < 1; }); break;
  case 29: cull(m, [&](V3 a, V3 b, size_t) { return (a.z + b.z) * .5f > C.z + A * .2f * sinf(t); }); break;
  case 30: cull(m, [&](V3, V3, size_t e) { return sinf(float(e) * .73f + t * 3) > -.1f; }); break;
  case 31: { Mesh3 x = m; for (int k = 1; k <= 4; k++) copyInto(m, x, {0, .08f * k, 0}, 1 - .08f * k, .13f * k + A * .03f); } break;
  case 32: { Mesh3 x = m; for (int k = 1; k < 6; k++) { float a = t * .3f + k * 2 * PI / 6; copyInto(m, x, {1.8f * cosf(a), 0, 1.8f * sinf(a)}, .35f, a); } } break;
  case 33: { Mesh3 x = m; for (int k = 1; k <= 4; k++) copyInto(m, x, {0, 0, 0}, powf(.55f, k), t * .1f * k); } break;
  case 34: { Mesh3 x = m; for (int k = 1; k <= 6; k++) copyInto(m, x, {-.12f * k, .04f * k, .08f * k}, 1 - .09f * k, -.06f * k); } break;
  case 35: mapv(m, [&](V3 p, size_t i) { float ph = t + float(i % 97) * .025f; p.x += A * .18f * sinf(ph + p.y); p.z += A * .18f * cosf(ph - p.x); return p; }); break;
  case 36: if (s && !s->v.empty()) { auto sv = s->v; auto se = s->e; unsigned base = (unsigned)m.v.size(); for (auto p : sv) m.v.push_back(p); for (auto i : se) m.e.push_back({base + i.a, base + i.b}); size_t n = std::min<size_t>(64, std::min<size_t>(base, sv.size())); for (size_t i = 0; i < n; i++) m.e.push_back({(unsigned)(i * base / n), base + (unsigned)(i * sv.size() / n)}); } break;
  case 37: if (s && !s->v.empty()) { auto sv = s->v; auto se = s->e; unsigned base = (unsigned)m.v.size(); for (auto p : sv) m.v.push_back(p); for (auto i : se) m.e.push_back({base + i.a, base + i.b}); for (unsigned i = 0; i < base; i += std::max(1u, base / 48u)) { float bd = 1e30f; unsigned bj = 0; for (unsigned j = 0; j < sv.size(); j++) { float d = vlen(vsub(m.v[i], sv[j])); if (d < bd) { bd = d; bj = j; } } m.e.push_back({i, base + bj}); } } break;
  case 38: { unsigned cc = (unsigned)m.v.size(); m.v.push_back(C); for (unsigned i = 0; i < cc; i += std::max(1u, cc / 96u)) m.e.push_back({cc, i}); } break;
  case 39: { unsigned n = (unsigned)m.v.size(); for (unsigned i = 0; i < n; i += std::max(1u, n / 80u)) { unsigned j = (i * 37 + 17) % n; if (i != j) m.e.push_back({i, j}); } } break;
  case 40: subdiv(m, 3, 0, c.seed); break;
  case 41: subdiv(m, 4, .09f * fabsf(A), c.seed); break;
  case 42: subdiv(m, 5, .05f * (1 + fabsf(A)), c.seed + 31); break;
  case 43: subdiv(m, 6, .15f * fabsf(A), c.seed + (uint32_t)(t * 8)); break;
  case 44: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float r = std::max(.15f, std::hypot(x.x, x.y)), f = A * .35f / (r * r + .2f); x.x *= 1 + f; x.y *= 1 + f; return vadd(C, x); }); break;
  case 45: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float r = std::max(.1f, hyp(x)), a = A * .6f / (r + .1f), co = cosf(a), si = sinf(a); return vadd(C, V3{x.x * co - x.z * si, x.y / (1 + .2f / r), x.x * si + x.z * co}); }); break;
  case 46: mapv(m, [&](V3 p, size_t) { for (float sx : {-1.f, 1.f}) { V3 h{C.x + sx * 1.1f, C.y, C.z}, d = vsub(p, h); float r = vlen(d); if (r < 1.2f) p = vadd(p, vmul(vnorm(d), A * .16f * (1.2f - r))); } return p; }); break;
  case 47: cull(m, [&](V3 a, V3 b, size_t) { return vlen(q(vmul(vadd(a, b), .5f))) > .65f + .15f * sinf(t); }); break;
  case 48: mapv(m, [&](V3 p, size_t) { p.x += A * .35f * sinf(p.y * p.y * 2 + t); p.z += A * .22f * sinf(p.x * 3 - p.y * 2); return p; }); break;
  case 49: mapv(m, [&](V3 p, size_t) { float d = 1 + A * .16f * p.z; if (fabsf(d) < .12f) d = d < 0 ? -.12f : .12f; return V3{p.x / d, p.y / d, p.z}; }); break;
  case 50: mapv(m, [&](V3 p, size_t) { float d = expf(clampf(A * .12f * p.z, -1.5f, 1.5f)); return V3{p.x * d, p.y * d, p.z}; }); break;
  case 51: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float r = vlen(x); return vadd(C, vmul(x, (1 + A * .12f * r * r) / (1 + .04f * r * r))); }); break;
  case 52: { Mesh3 x = m; mapv(m, [&](V3 p, size_t) { p.x -= .08f * A; return p; }); copyInto(m, x, {.16f * A, 0, 0}, 1, 0); } break;
  case 53: { Mesh3 x = m; copyInto(m, x, {.04f * A, 0, 0}, 1.01f, .012f * A); copyInto(m, x, {-.04f * A, 0, 0}, .99f, -.012f * A); } break;
  case 54: mapv(m, [&](V3 p, size_t) { return vadd(C, vmul(q(p), 1 + A * .35f * c.bass)); }); break;
  case 55: { EffectContext x = c; x.amount = A * (.2f + 2 * c.mid); return applyEffect(m, 1, x, s); }
  case 56: mapv(m, [&](V3 p, size_t) { float f = p.y > C.y ? c.treble : c.bass; return vadd(C, vmul(q(p), 1 + A * .25f * f)); }); break;
  case 57: if (c.beat > .4f) { EffectContext x = c; x.amount = A * c.beat; return applyEffect(m, 23, x, s); } break;
  case 58: mapv(m, [&](V3 p, size_t) { return vadd(C, vmul(q(p), 1 + A * .3f * c.bass * sinf(t * 2))); }); break;
  case 59: { EffectContext x = c; x.amount = A * c.treble; return applyEffect(m, 6, x, s); }
  case 60: mapv(m, [&](V3 p, size_t) { V3 x = q(p); float a = atan2f(x.z, x.x), f = c.bass * sinf(a * 3) + c.mid * sinf(a * 7) + c.treble * sinf(a * 13); return vadd(C, vmul(x, 1 + A * .12f * f)); }); break;
  case 61: mapv(m, [&](V3 p, size_t) { V3 v{sinf(p.y + t) - cosf(p.z), sinf(p.z - t) - cosf(p.x), sinf(p.x + t) - cosf(p.y)}; return vadd(p, vmul(v, A * .12f)); }); break;
  case 62: mapv(m, [&](V3 p, size_t) { V3 v{cosf(p.y) - sinf(p.z), cosf(p.z) - sinf(p.x), cosf(p.x) - sinf(p.y)}; return vadd(p, vmul(v, A * .16f)); }); break;
  case 63: mapv(m, [&](V3 p, size_t) { p.x += A * .2f * sinf(p.y * 2 + sinf(p.z * 3 + t)); p.y += A * .2f * sinf(p.z * 2 + sinf(p.x * 3 - t)); p.z += A * .2f * sinf(p.x * 2 + sinf(p.y * 3 + t)); return p; }); break;
  }
  sanitize(m);
  if (m.e.empty()) m = std::move(original);
  return meshOK(m);
}

EffectRecipe recipe(int id) {
  id = wrap(id, recipeCount());
  EffectRecipe r{};
  auto S = [](int fx, float a, float ts = 1, float to = 0, float so = 0, bool sec = false) { return EffectStage{fx, a, ts, to, so, sec}; };
  auto set = [&](std::string_view n, std::initializer_list<EffectStage> s) { r.name = std::string(n); r.stageCount = std::min<int>(6, (int)s.size()); int i = 0; for (auto& x : s) if (i < 6) r.stages[i++] = x; };
  switch (id) {
  case 0: set("Raw", {}); break;
  case 1: set("Singularity Bloom", {S(11, .8f), S(50, .55f), S(5, .45f), S(32, .7f)}); break;
  case 2: set("Impossible Cathedral", {S(17, 1), S(12, .7f), S(48, .65f), S(39, .45f)}); break;
  case 3: set("Quantum Shatter", {S(23, .7f), S(43, .7f), S(35, .55f), S(6, .2f)}); break;
  case 4: set("Hyper Echo", {S(51, .7f), S(34, .75f), S(13, .55f), S(5, .25f)}); break;
  case 5: set("Wormhole Lattice", {S(47, .9f), S(18, .8f), S(38, .6f), S(33, .35f)}); break;
  case 6: set("Spectral Organism", {S(62, .8f), S(60, 1), S(15, .4f), S(31, .45f)}); break;
  case 7: set("Topology Storm", {S(22, .65f), S(42, .5f), S(29, .8f), S(36, .7f, 1, 0, 0, true)}); break;
  case 8: set("Recursive Reactor", {S(33, .7f), S(44, .45f), S(10, .5f), S(4, .4f)}); break;
  case 9: set("Kaleido Collapse", {S(17, 1), S(20, .8f), S(25, .7f), S(13, .5f)}); break;
  case 10: set("Black Star", {S(46, .9f), S(49, .7f), S(28, .8f), S(43, .35f)}); break;
  case 11: set("Interference Bridge", {S(37, .8f, 1, 0, 0, true), S(61, .55f), S(14, .35f), S(32, .3f)}); break;
  case 12: set("Crystal Fracture", {S(7, .8f), S(42, .7f), S(17, .7f), S(6, .12f)}); break;
  case 13: set("Mobius Lightning", {S(16, .8f), S(43, .6f), S(13, .5f), S(34, .3f)}); break;
  case 14: set("Event Scanner", {S(49, .7f), S(25, .8f), S(14, .55f), S(35, .3f)}); break;
  case 15: set("Dark Matter Flower", {S(45, .65f), S(19, .75f), S(63, .55f), S(39, .35f)}); break;
  case 16: set("Projective Failure", {S(50, .9f), S(48, .7f), S(9, .45f), S(30, .5f)}); break;
  case 17: set("Recursive Constellation", {S(33, .65f), S(39, .8f), S(5, .25f), S(14, .4f)}); break;
  case 18: set("Alien Transmission", {S(55, .8f), S(60, .8f), S(26, .7f), S(44, .35f)}); break;
  case 19: set("Fractal Orbit", {S(32, .55f), S(33, .5f), S(12, .4f), S(4, .25f)}); break;
  case 20: set("Dual Singularity", {S(36, .65f, 1, 0, 0, true), S(47, .75f), S(50, .5f), S(34, .25f)}); break;
  case 21: set("Wireframe Supernova", {S(21, .65f), S(43, .5f), S(32, .5f), S(19, .35f)}); break;
  case 22: set("Quasicrystal Ghost", {S(63, .7f), S(34, .6f), S(17, .55f), S(27, .45f)}); break;
  default: set("Dimensional Infection", {S(61, .55f), S(16, .5f), S(42, .45f), S(50, .4f), S(32, .3f)}); break;
  }
  return r;
}
int recipeCount() { return 24; }
std::string_view recipeName(int id) {
  static const char* names[] = {"Raw","Singularity Bloom","Impossible Cathedral","Quantum Shatter","Hyper Echo","Wormhole Lattice","Spectral Organism","Topology Storm","Recursive Reactor","Kaleido Collapse","Black Star","Interference Bridge","Crystal Fracture","Mobius Lightning","Event Scanner","Dark Matter Flower","Projective Failure","Recursive Constellation","Alien Transmission","Fractal Orbit","Dual Singularity","Wireframe Supernova","Quasicrystal Ghost","Dimensional Infection"};
  return names[wrap(id, recipeCount())];
}

EffectRecipe mutateRecipe(uint32_t seed, int depth) {
  auto h = [](uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; };
  EffectRecipe r{}; r.name = "Mutation"; r.stageCount = std::clamp(depth, 1, 6);
  uint32_t x = seed;
  for (int i = 0; i < r.stageCount; i++) {
    x = h(x + 0x9e3779b9u + (uint32_t)i);
    int fx = 1 + (x % (effectCount() - 1));
    float a = .2f + ((x >> 8) & 255) / 255.f * .8f;
    float ts = .55f + ((x >> 16) & 255) / 255.f * 1.4f;
    float off = ((x >> 24) & 255) / 255.f * 3.14159f;
    r.stages[i] = EffectStage{fx, a, ts, off, float(h(x + 0x9e3779b9u + (uint32_t)i)), (fx == 36 || fx == 37)};
  }
  return r;
}

bool applyRecipe(Mesh3& m, const EffectRecipe& r, const EffectContext& base, const Mesh3* secondary) {
  for (int i = 0; i < std::clamp(r.stageCount, 0, 6); i++) {
    const EffectStage& s = r.stages[i];
    EffectContext c = base;
    c.time = base.time * s.timeScale + s.timeOffset;
    c.amount = base.amount * s.amount;
    c.seed = base.seed + (uint32_t)s.seedOffset;
    if (!applyEffect(m, s.effect, c, s.useSecondary ? secondary : nullptr)) return false;
  }
  return true;
}
