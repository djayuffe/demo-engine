// GPU ray tracer for the mirrored dodecahedron.
//
// Three modes share one fragment shader:
//   'object' — the hand-held sculpture seen from outside. Rays hitting the
//              glass are handed to the Klein model, where straight lines are
//              hyperbolic geodesics, and continue inside the right-angled
//              dodecahedron of the {5,3,4} honeycomb.
//   'flat'   — standing inside a Euclidean mirrored dodecahedron (116.57°
//              dihedral angles): the reflections cannot tile, so they tear.
//   'hyper'  — standing inside the hyperbolic one (90° dihedral angles):
//              every reflection is exactly a neighbouring cell.
//
// Hyperbolic geometry uses the hyperboloid model with the Minkowski form
// <a,b> = a.x b.x + a.y b.y + a.z b.z − a.w b.w. A ray is
// p(s) = cosh(s) p + sinh(s) v, mirrors are reflections x ↦ x − 2<x,N>N,
// so the ray never leaves the fundamental cell and stays numerically exact.

import { getGL, program, FULLSCREEN_VS, emptyVAO, fitCanvas, runWhenVisible, orbitControl, adaptiveBudget } from './gl.js';
import { FACES, EDGES, KLEIN_T, flat } from './dodeca.js';

const FS = (mode) => `#version 300 es
precision highp float;
#define MODE_${mode === 'cube' ? 'FLAT' : mode.toUpperCase()} 1
#define MAXB 34

in vec2 vUv;
out vec4 outColor;

uniform vec2 uRes;
uniform float uTime;
uniform mat3 uCam;
uniform vec3 uRo;
uniform float uTanHalf;
uniform vec3 uU[12];        // Euclidean face normals (plane u·x = 1)
uniform vec4 uHN[12];       // hyperbolic face normals (spacelike, unit)
uniform vec3 uEA[30];
uniform vec3 uEB[30];
uniform float uT;           // Klein inradius
uniform vec4 uBoost;        // xyz axis, w rapidity (camera drift inside H^3)
uniform float uMirror;
uniform float uFog;
uniform float uGlow;
uniform int uNF;            // active Euclidean faces (12 dodecahedron, 6 cube)
uniform float uSinD;        // sin(dihedral angle) for Euclidean edge distance
uniform float uFX;          // strength of the grid / glyph layer
uniform vec4 uPulse[6];     // light pulses: (front radius, strength, hue, width) in optical length
uniform float uFlow;        // organic clock: runs faster when the object is excited
uniform float uEnergy;      // 0 calm … 1.5 excited

const vec3 MINT = vec3(0.50, 1.00, 0.93);
const vec3 BLUE = vec3(0.45, 0.62, 1.00);
const vec3 VIOLET = vec3(0.78, 0.55, 1.00);
const float SIN_DIHEDRAL = 0.894427191; // sin(116.565°) for the Euclidean edge distance

float mdot(vec4 a, vec4 b) { return dot(a.xyz, b.xyz) - a.w * b.w; }

vec3 hue(float d) {
  float x = 1.0 - exp(-d * 0.17);
  return mix(mix(MINT, BLUE, smoothstep(0.0, 0.55, x)), VIOLET, smoothstep(0.45, 1.0, x));
}

// Pixel-footprint-aware coverage of an LED strip of half-width w at distance e.
float lineCov(float e, float w, float fp) {
  fp = max(fp, 1e-5);
  return clamp((w - e) / fp + 0.5, 0.0, 1.0) * min(1.0, 2.0 * w / fp);
}

float hash(vec2 p) {
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

// ------------------------------------------------------------ the grid
// Every mirror carries a light-cycle grid and a rain of glyphs. Because the
// mirrors repeat, so does the pattern — out to infinity.
//
// Intrinsic layout. A face of the hyperbolic cell is itself a hyperbolic
// plane H². With F its centre and E1, E2 orthonormal tangents there, the
// face's own Beltrami–Klein coordinates are  k = (<P,E1>, <P,E2>) / −<P,F>.
// Klein chords are geodesics, and the chord k₁ = tanh a is the geodesic
// that meets the E1 axis perpendicularly at distance a. So the grid uses
// X = atanh k₁, Y = atanh k₂: two families of GEODESICS, evenly spaced in
// true hyperbolic length along their axes. Lines of one family never meet
// yet spread apart (they are ultraparallel) — straight lines bending away
// from each other, which flat mirrors cannot show. Line widths are true
// hyperbolic distances,  d = asinh |<P, E1 cosh a + F sinh a>|,  matched
// against the pixel footprint carried by the Jacobi field (cosh / sinh).
// On flat mirrors the same pattern is laid out with Euclidean coordinates,
// and parallel lines stay parallel.
//
// Cell address. Unfolding the reflections turns the ray straight and the
// mirror sequence into a walk through the tiling: cell = g·C with
// g = R_{i1} R_{i2} … (4×4 Lorentz reflections, or affine Euclidean ones).
// The image of the centre, g·O, names the cell, however the ray got there.
// In the {5,3,4} honeycomb adjacent mirrors commute (90°), so many mirror
// words reach the same room — and they now agree on its glyphs. The number
// of distinct rooms still grows exponentially with depth, so the text never
// repeats. In the flat cube the addresses collapse onto a lattice; in the
// flat dodecahedron the group is not discrete and every image is new.
const vec3 TRON = vec3(0.25, 0.85, 1.0);
const vec3 CODE = vec3(0.35, 1.0, 0.55);
const vec3 CODE_FAR = vec3(0.55, 0.6, 1.0);

float h21(vec2 p) { return hash(p * 1.37 + 17.1); }
float vnoise(vec2 p) {
  vec2 i = floor(p), f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  return mix(mix(h21(i), h21(i + vec2(1, 0)), f.x), mix(h21(i + vec2(0, 1)), h21(i + vec2(1, 1)), f.x), f.y);
}
vec3 palette(float h) {   // mint → blue → violet → rose, a closed loop
  return 0.55 + 0.45 * cos(6.2831853 * (h + vec3(0.42, 0.55, 0.68)));
}

uint pcg3(uvec3 v) {
  v = v * 1664525u + 1013904223u;
  v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
  v ^= v >> 16u;
  v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
  return v.x ^ v.y ^ v.z;
}
// Stable id of an unfolded cell centre (rounded to half units; neighbouring
// centres are ≥ 1.6 apart, so rounding never merges two cells).
float cellId(vec3 c) { return float(pcg3(uvec3(ivec3(round(c * 2.0)) + 32768)) & 0xFFFFu); }

void faceBasis(vec3 u, out vec3 t1, out vec3 t2) {
  vec3 ax = abs(u.y) < 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
  t1 = normalize(cross(ax, u));
  t2 = cross(u, t1);
}

const float G = 0.067;                   // grid pitch (intrinsic length)
const vec2 CS = vec2(0.023, 0.032);      // glyph cell (intrinsic length)

// q: intrinsic face coordinates; gd: distance to the nearest line of each family.
vec3 faceFX(vec2 q, vec2 gd, float fp, float addr, float depth) {
  fp = max(fp, 1e-5);
  float grid = max(lineCov(gd.x, 0.0022, fp), lineCov(gd.y, 0.0022, fp));
  // Light cycles ride the Y = const lines (geodesics); each lane its own speed.
  float lane = floor(q.y / G + 0.5);
  float sp = 0.25 + 0.5 * h21(vec2(lane, addr));
  float dir = h21(vec2(addr, lane + 3.0)) < 0.5 ? -1.0 : 1.0;
  float run = fract(dir * q.x * 1.2 - uFlow * sp + h21(vec2(lane * 1.3, addr)));
  float cyc = lineCov(gd.y, 0.003, fp) * pow(run, 14.0) * (1.0 + 6.0 * smoothstep(0.985, 1.0, run));

  // Glyph rain falls down the X = const geodesics.
  vec2 cell = floor(q / CS);
  vec2 f = fract(q / CS);
  float sc = 0.5 + 1.5 * h21(vec2(cell.x, addr));
  float head = 22.0 - fract(uFlow * sc * 0.09 + h21(vec2(addr, cell.x * 1.7))) * 44.0;
  float dy = cell.y - floor(head);
  float trail = dy >= 0.0 ? exp(-dy * 0.16) : 0.0;
  float glyphT = floor(uFlow * (2.0 + 6.0 * h21(cell + addr)));
  vec2 px = floor(f * vec2(5.0, 7.0));
  float inBox = step(0.5, px.x) * step(px.x, 3.5) * step(0.5, px.y) * step(px.y, 5.5);
  float bit = step(0.45, h21(px + cell * 7.31 + vec2(addr, glyphT)));
  float detail = smoothstep(CS.x * 0.35, CS.x * 0.08, fp);   // fade to mean when glyphs alias
  float glyph = mix(0.22, bit * inBox, detail);
  // Columns come and go with a slow noise field, so the rain drifts in
  // patches like weather instead of a uniform curtain.
  float patchy = vnoise(vec2(cell.x * 0.13 + addr * 0.71, uFlow * 0.05 + addr));
  float rain = trail * glyph * smoothstep(0.35, 0.7, patchy + 0.25 * uEnergy);
  float lead = dy >= 0.0 && dy < 1.0 ? glyph : 0.0;

  vec3 code = mix(CODE, CODE_FAR, smoothstep(1.0, 7.0, depth));   // distant rooms run cooler
  return TRON * grid * 0.8 + TRON * cyc * 2.6 + code * rain * 1.3 + vec3(0.85, 1.0, 0.9) * lead * 1.8;
}

// Hyperbolic face: P on face f of the cell (hyperboloid), fp in hyperbolic units.
vec3 faceFXH(vec4 P, int f, float fp, float addr, float depth) {
  vec3 u = uU[f], t1, t2;
  faceBasis(u, t1, t2);
  float s = inversesqrt(1.0 - uT * uT);
  vec4 F = vec4(uT * u, 1.0) * s;         // face centre
  vec4 E1 = vec4(t1, 0.0), E2 = vec4(t2, 0.0);
  float pf = mdot(P, F), p1 = mdot(P, E1), p2 = mdot(P, E2);
  vec2 k = vec2(p1, p2) / -pf;
  vec2 q = vec2(atanh(clamp(k.x, -0.999999, 0.999999)), atanh(clamp(k.y, -0.999999, 0.999999)));
  vec2 a = floor(q / G + 0.5) * G;        // nearest geodesic of each family
  vec2 gd = vec2(asinh(abs(cosh(a.x) * p1 + sinh(a.x) * pf)),
                 asinh(abs(cosh(a.y) * p2 + sinh(a.y) * pf)));
  return faceFX(q, gd, fp, addr, depth);
}

// Euclidean face: same pattern, scaled so a pitch matches the hyperbolic one.
vec3 faceFXE(vec3 p, int f, float fp, float addr, float depth) {
  vec3 t1, t2;
  faceBasis(uU[f], t1, t2);
  const float K = 0.9;
  vec2 q = vec2(dot(p, t1), dot(p, t2)) * K;
  vec2 gd = abs(fract(q / G + 0.5) - 0.5) * G;
  return faceFX(q, gd, fp * K, addr, depth);
}

// Light gathered by one bounce landing at edge distance e.
vec3 shade(float e, float total, float fp, float w, vec3 fx, float addr, inout float att) {
  float fog = exp(-total * uFog);
  float cov = lineCov(e, w, fp);
  vec3 c = hue(total);
  // Time of flight. Each pulse is a flash of the LEDs; an image at optical
  // distance d is seen d/c later, so the flash is a spherical wavefront
  // running outward room by room, leaving a fading wake behind it.
  float flash = 0.0;
  vec3 tint = vec3(0.0);
  for (int i = 0; i < 6; i++) {
    vec4 P = uPulse[i];
    if (P.y <= 0.0) continue;
    float d = total - P.x;
    float k = P.y * (exp(-d * d / (P.w * P.w)) + (d < 0.0 ? 0.22 * exp(d * 0.9) : 0.0));
    flash += k;
    tint += k * palette(P.z);
  }
  if (flash > 1e-4) c = mix(c, tint / flash, clamp(flash, 0.0, 1.0) * 0.7);
  // Every room breathes on its own phase: an organic shimmer, never in step.
  float breath = 0.82 + 0.18 * sin(uFlow * (0.6 + 0.25 * fract(addr * 0.618)) + addr * 2.399);
  flash = (1.0 + 1.5 * flash) * breath;
  vec3 col = att * fog * (fx * uFX * (1.0 - cov) * (0.55 + 0.45 * flash)) + att * fog * c * (cov * uGlow * flash
           + 0.11 * exp(-e * 13.0) * min(1.0, 4.0 * w / max(fp, 1e-5))
           + 0.006);
  att *= (1.0 - cov) * uMirror;
  return col;
}

// ---------------------------------------------------------------- hyperbolic
vec3 traceHyper(vec4 p, vec4 v, float fp0, float pixAng, int skip, bool shadeFirst, inout float att) {
  vec3 col = vec3(0.0);
  float total = 0.0;
  mat4 Gm = mat4(1.0);                  // unfolding isometry g
  float addr = cellId(vec3(0.0)), depth = 0.0;
  const float W = 0.028;
  for (int b = 0; b < MAXB; b++) {
    float e = 1e9, best = 1e9;
    int bi = -1;
    for (int i = 0; i < 12; i++) {
      if (i == skip) continue;
      vec4 N = uHN[i];
      float a = mdot(N, p);         // < 0 inside the cell
      float bb = mdot(N, v);
      e = min(e, -a);
      if (bb > 1e-7) {
        float r = -a / bb;
        if (r < 0.999999) {
          float s = atanh(r);
          if (s < best) { best = s; bi = i; }
        }
      }
    }
    if (skip >= 0 && (b > 0 || shadeFirst)) {
      float tc = min(total, 40.0);
      float fp = fp0 * cosh(tc) + pixAng * sinh(tc);
      vec3 fx = faceFXH(p, skip, fp, addr, depth);
      col += shade(asinh(max(e, 0.0)), total, fp, W, fx, addr, att);
      if (att < 0.004) break;
    }
    if (bi < 0) break;
    float ch = cosh(best), sh = sinh(best);
    vec4 np = ch * p + sh * v;
    v = sh * p + ch * v;
    p = np;
    total += best;
    vec4 N = uHN[bi];
    v -= 2.0 * mdot(v, N) * N;        // the mirror
    // Unfold: g ← g·R_i; the cell centre is g·O.
    Gm = Gm * (mat4(1.0) - 2.0 * outerProduct(N, N * vec4(1.0, 1.0, 1.0, -1.0)));
    vec4 cc = Gm[3];
    addr = cellId(cc.xyz);
    depth = acosh(max(cc.w, 1.0));
    p /= sqrt(max(-mdot(p, p), 1e-12));
    v += mdot(v, p) * p;
    v /= sqrt(max(mdot(v, v), 1e-12));
    skip = bi;
  }
  return col;
}

// ----------------------------------------------------------------- Euclidean
vec3 traceFlat(vec3 p, vec3 v, float pixAng, inout float att) {
  vec3 col = vec3(0.0);
  float total = 0.0;
  int skip = -1;
  mat3 A = mat3(1.0); vec3 bT = vec3(0.0);   // unfolding affine map g(x) = A x + b
  float addr = cellId(vec3(0.0)), depth = 0.0;
  const float W = 0.022;
  for (int b = 0; b < MAXB; b++) {
    float e = 1e9, best = 1e9;
    int bi = -1;
    for (int i = 0; i < 12; i++) {
      if (i >= uNF) break;
      if (i == skip) continue;
      float a = 1.0 - dot(uU[i], p);
      float bb = dot(uU[i], v);
      e = min(e, a);
      if (bb > 1e-7) {
        float s = a / bb;
        if (s < best) { best = s; bi = i; }
      }
    }
    if (skip >= 0) {
      vec3 fx = faceFXE(p, skip, pixAng * total, addr, depth);
      col += shade(e / uSinD, total, pixAng * total, W, fx, addr, att);
      if (att < 0.004) break;
    }
    if (bi < 0) break;
    p += v * best;
    total += best;
    v = reflect(v, uU[bi]);
    // g ← g∘R_i,  R_i(x) = (I − 2uuᵀ)x + 2u
    vec3 ui = uU[bi];
    bT += 2.0 * (A * ui);
    A = A * (mat3(1.0) - 2.0 * outerProduct(ui, ui));
    addr = cellId(bT * 0.6687);
    depth = length(bT) * 0.6687;
    skip = bi;
  }
  return col;
}

// Lorentz boost of rapidity d along unit spatial axis a.
vec4 boost(vec4 x, vec3 a, float d) {
  float ax = dot(a, x.xyz);
  float ch = cosh(d), sh = sinh(d);
  return vec4(x.xyz + ((ch - 1.0) * ax + sh * x.w) * a, ch * x.w + sh * ax);
}

// Closest approach between the ray and segment AB: (distance, ray parameter).
vec2 raySeg(vec3 ro, vec3 rd, vec3 A, vec3 B) {
  vec3 ba = B - A, oa = ro - A;
  float a = dot(ba, ba), b = dot(ba, rd), c = dot(ba, oa), d = dot(rd, oa);
  float den = a - b * b;
  float h = clamp((c - b * d) / max(den, 1e-8), 0.0, 1.0);
  vec3 q = A + ba * h;
  float t = max(dot(q - ro, rd), 0.0);
  return vec2(length(ro + rd * t - q), t);
}

void main() {
  vec2 frag = gl_FragCoord.xy;
  vec2 uv = (frag * 2.0 - uRes) / uRes.y;
  vec3 rd = normalize(uCam * vec3(uv * uTanHalf, -1.0));
  float pixAng = 2.0 * uTanHalf / uRes.y;
  vec3 col = vec3(0.0);
  float alpha = 1.0;
  float att = 1.0;

#if defined(MODE_OBJECT)
  vec3 ro = uRo;
  float tE = -1e9, tX = 1e9;
  int fIn = -1;
  for (int i = 0; i < 12; i++) {
    float dn = dot(uU[i], rd);
    float t = (1.0 - dot(uU[i], ro)) / dn;
    if (dn < 0.0) { if (t > tE) { tE = t; fIn = i; } }
    else tX = min(tX, t);
  }
  bool hit = tE < tX && tE > 0.0;

  // Bloom from the LED edges on the near side.
  float limit = hit ? tE + 0.06 : 1e9;
  vec3 halo = vec3(0.0);
  for (int i = 0; i < 30; i++) {
    vec2 r = raySeg(ro, rd, uEA[i], uEB[i]);
    if (r.y < limit) halo += hue(0.0) * (0.0016 / (r.x * r.x + 0.0016) * 0.07 + exp(-r.x * 22.0) * 0.05);
  }
  halo *= 0.7 + 0.3 * uGlow / 2.4;   // the halo breathes with the LEDs

  if (hit) {
    vec3 x = ro + rd * tE;
    vec3 n = uU[fIn];
    float e = 1e9;
    for (int i = 0; i < 12; i++) if (i != fIn) e = min(e, 1.0 - dot(uU[i], x));
    e /= SIN_DIHEDRAL;
    float fpE = tE * pixAng;
    float cov = lineCov(e, 0.024, fpE);
    float cosI = abs(dot(rd, n));
    float fres = 0.04 + 0.96 * pow(1.0 - cosI, 5.0);
    vec3 refl = reflect(rd, n);
    vec3 env = vec3(0.06, 0.10, 0.13) * (0.4 + 0.6 * smoothstep(-0.2, 0.9, refl.y))
             + vec3(0.5, 0.9, 1.0) * pow(max(dot(refl, normalize(vec3(0.5, 0.8, 0.3))), 0.0), 60.0) * 0.6;
    // Thin-film iridescence on the one-way coating: colour shifts with view angle.
    vec3 irid = 0.5 + 0.5 * cos(6.2831853 * (cosI * 1.7 + vec3(0.0, 0.33, 0.67) + uTime * 0.04));
    env *= mix(vec3(1.0), 0.4 + 1.2 * irid, 0.55);
    // Two small orbiting lights sweep glints across the facets (the star
    // filter in the composite pass turns them into sparkles).
    for (int g = 0; g < 2; g++) {
      float a = uTime * (0.55 + 0.3 * float(g)) + float(g) * 2.7;
      vec3 Lg = normalize(vec3(cos(a), 0.55 - 0.9 * float(g), sin(a)));
      env += vec3(1.0, 0.97, 0.92) * pow(max(dot(refl, Lg), 0.0), 900.0) * 9.0;
    }

    // Hand the ray to the Klein model: same straight line, now a geodesic.
    vec3 xk = x * uT;
    float k2 = dot(xk, xk);
    vec4 p = vec4(xk, 1.0) / sqrt(1.0 - k2);
    vec4 vv = vec4(rd, 0.0);
    vec4 v = vv + mdot(vv, p) * p;
    v /= sqrt(max(mdot(v, v), 1e-12));
    float fp0 = fpE * uT / (1.0 - k2);
    float ia = 1.0;
    vec3 inside = traceHyper(p, v, fp0, pixAng, fIn, false, ia);

    vec3 glass = vec3(0.012, 0.03, 0.04);
    col = (inside + glass) * (1.0 - fres) * (1.0 - cov) + env * fres + MINT * uGlow * cov;
    col += halo * 0.5;
    alpha = 1.0;
  } else {
    col = halo;
    alpha = 0.0;
  }
#else
  #if defined(MODE_FLAT)
    col = traceFlat(uRo, rd, pixAng, att);
  #else
    vec4 p = boost(vec4(0.0, 0.0, 0.0, 1.0), uBoost.xyz, uBoost.w);
    vec4 v = boost(vec4(rd, 0.0), uBoost.xyz, uBoost.w);
    col = traceHyper(p, v, 0.0, pixAng, -1, false, att);
  #endif
#endif

  col = 1.0 - exp(-col * 1.35);
  col += (hash(frag + fract(uTime) * 91.7) - 0.5) / 255.0;
  float a = max(alpha, clamp(max(col.r, max(col.g, col.b)), 0.0, 1.0));
  outColor = vec4(col, a);
}`;

// Composite: chromatic fringe, mip-chain bloom and a four-point star filter.
const POST_FS = `#version 300 es
precision highp float;
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uImg;
uniform vec2 uRes;
uniform float uBloom;
uniform float uStar;
uniform float uBeat;
vec3 bright(vec3 c, float thr) { return max(c - thr, 0.0); }
void main() {
  vec2 uv = vUv;
  vec2 dc = uv - 0.5;
  // Lateral chromatic aberration, stronger toward the frame edge.
  vec2 ca = dc * dot(dc, dc) * 0.012;
  vec4 base = texture(uImg, uv);
  base.r = texture(uImg, uv + ca).r;
  base.b = texture(uImg, uv - ca).b;

  vec3 bloom = vec3(0.0);
  float ws = 0.0;
  for (int i = 0; i < 6; i++) {
    float lod = 1.0 + float(i);
    float w = 1.0 / (1.0 + float(i) * 0.6);
    bloom += bright(textureLod(uImg, uv, lod).rgb, 0.22 - 0.025 * float(i)) * w;
    ws += w;
  }
  bloom /= ws;

  // Four-point star: sparse taps along two diagonals on a fine mip.
  vec3 star = vec3(0.0);
  vec2 px = 1.0 / uRes;
  for (int i = 1; i <= 10; i++) {
    float d = float(i) * 3.2;
    float w = exp(-float(i) * 0.32);
    vec2 a = vec2(d, d * 0.18) * px, b = vec2(-d * 0.18, d) * px;
    star += (bright(textureLod(uImg, uv + a, 1.0).rgb, 0.55) + bright(textureLod(uImg, uv - a, 1.0).rgb, 0.55)
           + bright(textureLod(uImg, uv + b, 1.0).rgb, 0.55) + bright(textureLod(uImg, uv - b, 1.0).rgb, 0.55)) * w;
  }

  vec3 add = bloom * uBloom * (0.8 + 0.5 * uBeat) + star * uStar;
  vec3 c = base.rgb + add * (1.0 - base.rgb);   // screen blend: glow never washes out detail
  float a = max(base.a, clamp(max(c.r, max(c.g, c.b)), 0.0, 1.0));
  outColor = vec4(min(c, vec3(a)), a);
}`;

// A small nervous system for each chamber. It fires light pulses on its own
// at irregular (Poisson) intervals, faster when excited; a tap fires a strong
// one. Turning the object excites it, and it calms back to a resting state.
// Its clock ("flow") speeds up with excitement, so the rain and the light
// cycles quicken smoothly instead of jumping.
function createMind(canvas, ctl) {
  const pulses = [];
  let energy = 0.25, flow = 0, next = 0.6 + Math.random() * 1.5;
  const fire = (strength, user) => {
    pulses.push({ r: 0, s: strength, h: Math.random(), w: 0.22 + Math.random() * 0.3, v: 1.2 + Math.random() * 0.8 });
    if (pulses.length > 6) pulses.shift();
    energy = Math.min(1.5, energy + (user ? 0.55 : 0.08));
  };
  canvas.addEventListener('pointerdown', () => fire(2.2, true));
  const buf = new Float32Array(24);
  return {
    update(dt, t) {
      const motion = Math.min(1, Math.hypot(ctl.vy, ctl.vp) * 0.35 + (ctl.dragging ? 0.25 : 0));
      energy += (0.25 + motion - energy) * (1 - Math.exp(-dt * 1.2));
      flow += dt * (0.55 + 0.9 * energy);
      next -= dt * (0.5 + 1.6 * energy);
      if (next <= 0) { fire(0.55 + Math.random() * 0.75, false); next = -Math.log(1 - Math.random()) * 2.6; }
      buf.fill(0);
      let live = 0, glow = 0;
      for (let i = pulses.length - 1; i >= 0; i--) {
        const p = pulses[i];
        p.r += p.v * dt;
        p.s *= Math.exp(-dt * 0.16);
        if (p.s < 0.03 || p.r > 40) { pulses.splice(i, 1); continue; }
      }
      pulses.forEach((p, i) => {
        buf.set([p.r, p.s, p.h, p.w], i * 4);
        live++;
        glow += p.s * Math.exp(-p.r * 0.6);   // near pulses brighten the whole view
      });
      const breath = 0.5 + 0.5 * (0.6 * Math.sin(t * 0.83) + 0.4 * Math.sin(t * 0.31 + 1.3));
      return { buf, energy, flow, breath, glow, live };
    },
  };
}

function mat3LookFrom(yaw, pitch) {
  const cy = Math.cos(yaw), sy = Math.sin(yaw), cp = Math.cos(pitch), sp = Math.sin(pitch);
  // Columns: right, up, back (camera looks down −back).
  const back = [sy * cp, sp, cy * cp];
  const right = [cy, 0, -sy];
  const up = [-sy * sp, cp, -cy * sp];
  return { m: new Float32Array([...right, ...up, ...back]), back };
}

const MODES = {
  object: { bloom: 0.75, star: 0.12, fx: 1.6, fov: 30, dist: 6.4, glow: 2.4, fog: 0.05, mirror: 0.86, pixels: 1.3e6 },
  cube:   { bloom: 0.5, star: 0.04, fx: 0.8, fov: 78, glow: 2.0, fog: 0.06, mirror: 0.84, pixels: 4e5, faces: [[1,0,0],[-1,0,0],[0,1,0],[0,-1,0],[0,0,1],[0,0,-1]], sinD: 1 },
  flat:   { bloom: 0.5, star: 0.04, fx: 0.8, fov: 78, glow: 2.0, fog: 0.06, mirror: 0.84, pixels: 5e5 },
  hyper:  { bloom: 0.5, star: 0.04, fx: 0.8, fov: 78, glow: 2.0, fog: 0.05, mirror: 0.86, pixels: 5e5 },
};

export function createChamber(canvas, mode) {
  const cfg = MODES[mode];
  const gl = getGL(canvas);
  const { program: prog, u } = program(gl, FULLSCREEN_VS, FS(mode));
  const vao = emptyVAO(gl);
  const post = program(gl, FULLSCREEN_VS, POST_FS);

  // Offscreen target with a mip chain for bloom.
  let img = null, fbo = gl.createFramebuffer(), tw = 0, th = 0;
  function target(w, h) {
    if (w === tw && h === th && img) return;
    tw = w; th = h;
    if (img) gl.deleteTexture(img);
    img = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, img);
    gl.texStorage2D(gl.TEXTURE_2D, Math.floor(Math.log2(Math.max(w, h))) + 1, gl.RGBA8, w, h);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, img, 0);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
  }

  const s = 1 / Math.sqrt(1 - KLEIN_T * KLEIN_T);
  const hn = new Float32Array(FACES.flatMap((f) => [f[0] * s, f[1] * s, f[2] * s, KLEIN_T * s]));

  gl.useProgram(prog);
  const faces = cfg.faces || FACES;
  gl.uniform3fv(u.uU, flat([...faces, ...Array(12 - faces.length).fill([0, 0, 0])]));
  if (u.uNF) gl.uniform1i(u.uNF, faces.length);
  if (u.uSinD) gl.uniform1f(u.uSinD, cfg.sinD ?? 0.894427191);
  gl.uniform4fv(u.uHN, hn);
  if (u.uEA) gl.uniform3fv(u.uEA, flat(EDGES.map((e) => e[0])));
  if (u.uEB) gl.uniform3fv(u.uEB, flat(EDGES.map((e) => e[1])));
  if (u.uT) gl.uniform1f(u.uT, KLEIN_T);
  gl.uniform1f(u.uMirror, cfg.mirror);
  gl.uniform1f(u.uFog, cfg.fog);
  gl.uniform1f(u.uGlow, cfg.glow);
  if (u.uFX) gl.uniform1f(u.uFX, cfg.fx);
  gl.uniform1f(u.uTanHalf, Math.tan((cfg.fov * Math.PI) / 360));

  const ctl = orbitControl(canvas, {
    yaw: mode === 'object' ? 0.6 : 0.3,
    pitch: mode === 'object' ? 0.35 : 0.12,
    autoSpin: mode === 'object' ? 0.16 : 0.06,
  });
  canvas.style.cursor = 'grab';
  const budget = adaptiveBudget(cfg.pixels);
  const mind = createMind(canvas, ctl);

  runWhenVisible(canvas, (t, dt) => {
    ctl.update(dt);
    fitCanvas(canvas, budget.update(dt));
    const W = canvas.width, H = canvas.height;
    target(W, H);
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    gl.viewport(0, 0, W, H);
    gl.useProgram(prog);
    gl.bindVertexArray(vao);
    const m8 = mind.update(dt, t);
    const beat = Math.min(1.2, 0.35 * m8.breath + m8.glow);
    gl.uniform1f(u.uGlow, cfg.glow * (0.84 + 0.14 * m8.breath + 0.22 * Math.min(1, m8.glow)));
    gl.uniform4fv(u.uPulse, m8.buf);
    gl.uniform1f(u.uFlow, m8.flow);
    if (u.uEnergy) gl.uniform1f(u.uEnergy, m8.energy);

    const { m, back } = mat3LookFrom(ctl.yaw, ctl.pitch);
    gl.uniformMatrix3fv(u.uCam, false, m);
    gl.uniform2f(u.uRes, canvas.width, canvas.height);
    gl.uniform1f(u.uTime, t);

    if (mode === 'object') {
      gl.uniform3f(u.uRo, back[0] * cfg.dist, back[1] * cfg.dist, back[2] * cfg.dist);
    } else if (mode === 'flat' || mode === 'cube') {
      const r = 0.22;
      gl.uniform3f(u.uRo, Math.sin(t * 0.21) * r, Math.sin(t * 0.17 + 1) * r * 0.6, Math.cos(t * 0.13) * r);
    } else {
      const ax = [Math.cos(t * 0.11), Math.sin(t * 0.07) * 0.5, Math.sin(t * 0.11)];
      const l = Math.hypot(...ax);
      gl.uniform4f(u.uBoost, ax[0] / l, ax[1] / l, ax[2] / l, 0.28 * Math.sin(t * 0.23));
    }
    gl.drawArrays(gl.TRIANGLES, 0, 3);

    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.bindTexture(gl.TEXTURE_2D, img);
    gl.generateMipmap(gl.TEXTURE_2D);
    gl.useProgram(post.program);
    gl.uniform1i(post.u.uImg, 0);
    gl.uniform2f(post.u.uRes, W, H);
    gl.uniform1f(post.u.uBloom, cfg.bloom);
    gl.uniform1f(post.u.uStar, cfg.star);
    gl.uniform1f(post.u.uBeat, beat);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
  });
}
