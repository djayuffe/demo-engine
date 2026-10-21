// The instrument: a physically based, progressive, spectral ray tracer for the
// sculpture as it would actually be built.
//
// Physical model (object space, unit = Poincaré-ball radius):
//   • Medium. Hyperbolic GRIN: n(x) = n_c / (1 − |x|²). This is the conformal
//     factor of the Poincaré ball, so by Fermat's principle light in it follows
//     hyperbolic geodesics exactly. Lowest index at the centre, rising to the
//     corners (n_c = 1.30 → 1.85 at the vertices, within plastic range).
//     Uniform: n = n_c everywhere (an ordinary glass dodecahedron).
//   • Horizon (optional): multiplies n by the Schwarzschild optical index in
//     isotropic coordinates, (1 + a/r)³ / (1 − a/r), which diverges at r = a.
//   • Faces. Curved: spheres orthogonal to the ball — true hyperbolic planes —
//     giving the right-angled dodecahedron of the {5,3,4} honeycomb.
//     Flat: the planes through the same 20 vertices (what a printer would make
//     with straight sides); these are not hyperbolic planes, so seams appear.
//   • One-way film on every face: reflectance R, transmittance T. LED strips
//     of fixed physical width along the 30 edges.
//   • Dispersion. The two blended plastics have Cauchy dispersion, so at
//     wavelength λ the index is affine in the design index:
//     n_λ = n + D·s(λ)·(n − 1),  s = (λ0/λ)² − 1. White LEDs are rendered by
//     hero-wavelength spectral sampling; monochrome LEDs sit at λ0.
//   • Absorption. Beer–Lambert loss along every path, stronger toward blue
//     (∝ 1 + 0.9 (450/λ)⁴), as in real clear plastics.
//   • Printed layers. Fresnel partial reflection + TIR at each shell wall,
//     sampled stochastically per path.
//   • Plinth (outside view). A dark gloss surface below: inverse-square LED
//     glow pooling under the object plus a glossy (jittered) mirror image.
//   • Display. Mip-chain bloom approximates the camera's long-tailed glare.
//   • Surround. Air (1.00) or water (1.33). Entry uses Snell's law and the
//     full dielectric Fresnel equations; total internal reflection included.
//
// Integration: ray equation dT/ds = ∇ln n − (∇ln n·T)T with an adaptive RK2
// (midpoint) step bounded by the local ray-curvature radius n/|∇n| and the
// distance to the nearest face. Face crossings are located inside the step and
// mirrored about the local surface normal. Samples accumulate additively into
// an RGBA16F buffer while the view is still; a display pass resolves them.

import { getGL, program, FULLSCREEN_VS, emptyVAO, fitCanvas, runWhenVisible, orbitControl } from './gl.js';
import { FACES, VERTICES, KLEIN_T, flat } from './dodeca.js';

const t = KLEIN_T;
const RHO_F = t / (1 + Math.sqrt(1 - t * t));        // Poincaré radius of face centres
const SPH_C = (RHO_F + 1 / RHO_F) / 2;               // face sphere centre distance
const SPH_R = (1 / RHO_F - RHO_F) / 2;               // face sphere radius (C² = 1 + R²)
const VR = Math.hypot(...VERTICES[0]);               // Euclidean circum/in ratio
const KV = t * VR;
const RHO_V = KV / (1 + Math.sqrt(1 - KV * KV));     // Poincaré radius of the vertices
const D_FLAT = RHO_V / VR;                           // plane distance of flat faces

export const PHYS = { nCentre: 1.3, nCorner: 1.3 / (1 - RHO_V * RHO_V), layers: 7 };
// Stepped (printed) gradient: the index is quantised into PHYS.layers shells of
// equal ln n, each a uniform plastic. Rays run straight inside a shell and
// refract by Snell's law (with total internal reflection) at every shell wall.
const LAYER_D = Math.log(PHYS.nCorner / PHYS.nCentre) / PHYS.layers;

const TRACE_FS = `#version 300 es
precision highp float;
precision highp int;
out vec4 outColor;

uniform vec2 uRes;
uniform mat3 uCam;
uniform vec3 uRo;
uniform float uTanHalf;
uniform uint uFrame;
uniform vec3 uU[12];
uniform int uMedium;   // 0 uniform, 1 hyperbolic
uniform int uHole;
uniform int uCurved;
uniform int uWhite;
uniform int uInside;
uniform float uNout;
uniform int uSteps;    // 1: printed in discrete index layers

const float NC = ${PHYS.nCentre.toFixed(4)};
const float RHO_V = ${RHO_V.toFixed(6)};
const float D_FLAT = ${D_FLAT.toFixed(6)};
const float SC = ${SPH_C.toFixed(6)};
const float SR = ${SPH_R.toFixed(6)};
const float LAYER_D = ${LAYER_D.toFixed(6)};
const float LAYER_MAX = ${(PHYS.layers - 1).toFixed(1)};
const float KT = ${KLEIN_T.toFixed(7)};      // Klein inradius of the right-angled cell
const float INV_SIN_DIH = 1.118033989;  // 1 / sin(116.565°): in-face width of an LED on a flat face
const float HOLE_A = 0.055;
const float DISP = 0.05;
const float L0 = 532.0;
const float LED_W = 0.0075;
const float FILM_R = 0.86;
const float FILM_T = 0.12;
const float ABSORB = 0.07;          // Beer–Lambert loss of the blend, per unit length at 450 nm+
const float FLOOR_Y = -0.64;        // display plinth under the object (outside view)
const vec3 KEY = vec3(-0.4, 0.85, 0.35);
const int MAX_STEPS = 520;
const int MAX_BOUNCE = 40;
const vec3 MINT = vec3(0.42, 1.0, 0.88);

// ---------------------------------------------------------------- random
uint pcg(inout uint s) {
  s = s * 747796405u + 2891336453u;
  uint w = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
  return (w >> 22u) ^ w;
}
float rnd(inout uint s) { return float(pcg(s)) * (1.0 / 4294967296.0); }
uint gSeed;     // per-sample stream shared by the tracer's stochastic events
float gSigma;   // absorption coefficient of the plastic at the current λ

// ---------------------------------------------------------------- geometry
float faceF(int i, vec3 x) {
  if (uCurved == 1) return SR - length(x - SC * uU[i]);
  return dot(uU[i], x) - D_FLAT;
}
vec3 faceN(int i, vec3 x) { // outward from the cell
  if (uCurved == 1) return normalize(SC * uU[i] - x);
  return uU[i];
}
float objF(vec3 x, out int fi) {
  float m = -1e9; fi = 0;
  for (int i = 0; i < 12; i++) { float f = faceF(i, x); if (f > m) { m = f; fi = i; } }
  if (uCurved == 1) m = max(m, length(x) - RHO_V * 1.0005);
  return m;
}

// ---------------------------------------------------------------- medium
float gDs; // dispersion offset D·s(λ) for the current sample

bool layered() { return uSteps == 1 && uMedium == 1; }
float layerOf(float r2) { return min(floor(-log(max(1.0 - r2, 1e-4)) / LAYER_D), LAYER_MAX); }
float layerR(float j) { return sqrt(1.0 - exp(-j * LAYER_D)); }
float layerN(float j) { return NC * exp((j + 0.5) * LAYER_D); }

float designN(vec3 x) {
  float r2 = dot(x, x);
  float n = NC;
  if (uMedium == 1) n = uSteps == 1 ? layerN(layerOf(r2)) : NC / max(1.0 - r2, 1e-3);
  if (uHole == 1) {
    float r = max(sqrt(r2), HOLE_A * 1.0001);
    float q = HOLE_A / r;
    n *= pow(1.0 + q, 3.0) / (1.0 - q);
  }
  return n;
}
vec3 gradLnDesign(vec3 x) {
  float r2 = dot(x, x);
  vec3 g = (uMedium == 1 && uSteps == 0) ? 2.0 * x / max(1.0 - r2, 1e-3) : vec3(0.0);
  if (uHole == 1) {
    float r = max(sqrt(r2), HOLE_A * 1.0001);
    float a = HOLE_A;
    g += (-3.0 * a / (r * (r + a)) - a / (r * (r - a))) * x / r;
  }
  return g;
}
float nLambda(vec3 x) { float n = designN(x); return n + gDs * (n - 1.0); }
float holeFactor(vec3 x) {
  if (uHole == 0) return 1.0;
  float r = max(length(x), HOLE_A * 1.0001), q = HOLE_A / r;
  return pow(1.0 + q, 3.0) / (1.0 - q);
}
float layerNL(float j, vec3 x) { float n = layerN(j) * holeFactor(x); return n + gDs * (n - 1.0); }
vec3 gradLn(vec3 x) {
  float n = designN(x);
  // ∇ln n_λ = (1 + D s) ∇n / n_λ = (1 + D s) n ∇ln n / n_λ
  return gradLnDesign(x) * ((1.0 + gDs) * n / (n + gDs * (n - 1.0)));
}
vec3 curveRate(vec3 x, vec3 T) { vec3 g = gradLn(x); return g - dot(g, T) * T; }

// ---------------------------------------------------------------- spectrum
vec3 spectrum(float l) {
  float r = exp(-pow((l - 605.0) / 50.0, 2.0)) + 0.3 * exp(-pow((l - 430.0) / 22.0, 2.0));
  float g = exp(-pow((l - 545.0) / 45.0, 2.0));
  float b = exp(-pow((l - 455.0) / 32.0, 2.0));
  return vec3(r, g, b);
}
const vec3 SPEC_NORM = vec3(3.0094, 3.7613, 5.3294); // 1 / mean over 400–700 nm → white integrates to (1,1,1)

// ---------------------------------------------------------------- optics
float fresnel(float cosi, float n1, float n2) {
  float eta = n1 / n2;
  float sint2 = eta * eta * (1.0 - cosi * cosi);
  if (sint2 >= 1.0) return 1.0;
  float cost = sqrt(1.0 - sint2);
  float rs = (n1 * cosi - n2 * cost) / (n1 * cosi + n2 * cost);
  float rp = (n2 * cosi - n1 * cost) / (n2 * cosi + n1 * cost);
  return 0.5 * (rs * rs + rp * rp);
}

float edgeDist(vec3 x, int fi) {
  float e = 1e9;
  for (int j = 0; j < 12; j++) if (j != fi) e = min(e, -faceF(j, x));
  // Curved faces meet at 90° (the ball is conformal), so distance to the
  // neighbouring face is the in-face distance to the edge. Flat faces meet
  // at the Euclidean dihedral angle and need the 1/sin correction.
  return uCurved == 1 ? e : e * INV_SIN_DIH;
}
float ledEmit(float e) {
  return smoothstep(LED_W, LED_W * 0.55, e) * 9.0 + exp(-e * 90.0) * 0.35 + exp(-e * 18.0) * 0.03;
}

// ---------------------------------------------------------------- exact path
// For the design medium at the design wavelength (no horizon, smooth blend)
// n = n_c / (1 − |x|²) is exactly the Poincaré-ball metric, so rays are
// hyperbolic geodesics and need no numerical integration at all. We lift to
// the hyperboloid (⟨a,b⟩ = a·b − a_w b_w), where a geodesic is
// P(s) = cosh s·P + sinh s·V. Every face, curved or flat, is a level set
// ⟨M,P⟩ = c of a linear function there:
//   curved (hyperbolic plane):  u·k = t  in Klein  →  M = (u, t),  c = 0
//   flat (Euclidean plane):     u·x = D  in ball   →  M = (u, D),  c = D
// so A cosh s + B sinh s = c is a quadratic in e^s, solved in closed form.
// Reflection uses the face's Minkowski normal projected to the tangent
// space, which is the Euclidean mirror in the (conformal) ball.
float mdot(vec4 a, vec4 b) { return dot(a.xyz, b.xyz) - a.w * b.w; }
vec3 toBall(vec4 P) { return P.xyz / (1.0 + P.w); }
vec4 faceM(int i) { return vec4(uU[i], uCurved == 1 ? KT : D_FLAT); }
float faceC() { return uCurved == 1 ? 0.0 : D_FLAT; }

float traceExact(vec3 x, vec3 T, int skip) {
  float r2 = dot(x, x), q = 1.0 - r2, dr2 = 2.0 * dot(x, T);
  vec4 P = vec4(2.0 * x, 1.0 + r2) / q;
  vec4 V = vec4(2.0 * T / q + 2.0 * x * dr2 / (q * q), 2.0 * dr2 / (q * q));
  V /= sqrt(max(mdot(V, V), 1e-12));
  float c = faceC();
  float L = 0.0, att = 1.0;
  for (int b = 0; b <= MAX_BOUNCE; b++) {
    float best = 1e9; int bi = -1;
    for (int i = 0; i < 12; i++) {
      vec4 M = faceM(i);
      float A = mdot(M, P), B = mdot(M, V);
      // (A + B) e² − 2c e + (A − B) = 0,  e = exp(s)
      float qa = A + B, qb = -2.0 * c, qc = A - B;
      float r0 = -1.0, r1 = -1.0;
      if (abs(qa) < 1e-9) { if (abs(qb) > 1e-12) r0 = -qc / qb; }
      else {
        float disc = qb * qb - 4.0 * qa * qc;
        if (disc >= 0.0) {
          float sq = sqrt(disc);
          // numerically stable pair
          float k = -0.5 * (qb + (qb >= 0.0 ? sq : -sq));
          r0 = k / qa; r1 = abs(k) > 1e-12 ? qc / k : -1.0;
        }
      }
      for (int j = 0; j < 2; j++) {
        float e = j == 0 ? r0 : r1;
        if (e <= 1.0) continue;
        float sh = 0.5 * (e - 1.0 / e), ch = 0.5 * (e + 1.0 / e);
        float s = log(e);
        if (A * sh + B * ch <= 0.0) continue;            // entering, not leaving
        if (i == skip && s < 1e-4) continue;             // the face we just left
        if (s < best) { best = s; bi = i; }
      }
    }
    if (bi < 0) break;
    float ch = cosh(best), sh = sinh(best);
    float chm = cosh(0.5 * best), shm = sinh(0.5 * best);
    vec3 x0 = toBall(P), xm = toBall(chm * P + shm * V);
    vec4 P1 = ch * P + sh * V;
    V = sh * P + ch * V;
    P = P1;
    vec3 xh = toBall(P);
    // Beer–Lambert over the physical (Euclidean) arc length in the part.
    att *= exp(-gSigma * (length(xm - x0) + length(xh - xm)));

    float e = edgeDist(xh, bi);
    L += att * ledEmit(e);
    att *= (1.0 - smoothstep(LED_W, LED_W * 0.55, e)) * FILM_R;
    if (att < 0.003) break;

    vec4 M = faceM(bi);
    vec4 n = M + mdot(M, P) * P;
    V -= 2.0 * mdot(V, n) / mdot(n, n) * n;
    P /= sqrt(max(-mdot(P, P), 1e-12));
    V += mdot(V, P) * P;
    V /= sqrt(max(mdot(V, V), 1e-12));
    skip = bi;
  }
  return L;
}

// March inside the medium; returns gathered LED radiance (scalar, at λ).
float traceInside(vec3 x, vec3 T, int skip) {
  if (uMedium == 1 && uHole == 0 && uSteps == 0 && gDs == 0.0) return traceExact(x, T, skip);
  float L = 0.0, att = 1.0;
  int bounces = 0;
  for (int s = 0; s < MAX_STEPS; s++) {
    // Adaptive step: a fraction of the curvature radius, and never past a face by much.
    vec3 g = gradLn(x);
    float hc = 0.16 / (length(g) + 1.5);
    int fi;
    float dFace = -objF(x, fi);
    float h = clamp(min(hc, dFace + 0.006), 0.0015, 0.07);
    float j0 = 0.0;
    if (layered()) {
      // Never step across more than one shell wall.
      float r = length(x);
      j0 = layerOf(r * r);
      float dB = j0 < LAYER_MAX ? layerR(j0 + 1.0) - r : 1.0;
      if (j0 > 0.0) dB = min(dB, r - layerR(j0));
      h = clamp(min(h, dB + 0.003), 0.0015, 0.07);
    }
    vec3 k1 = g - dot(g, T) * T;
    vec3 xm = x + 0.5 * h * T;
    vec3 Tm = normalize(T + 0.5 * h * k1);
    vec3 x1 = x + h * Tm;
    vec3 T1 = normalize(T + h * curveRate(xm, Tm));
    att *= exp(-gSigma * h);  // the plastic is not perfectly clear

    // Earliest face crossing inside this step.
    float best = 2.0; int bi = -1;
    for (int i = 0; i < 12; i++) {
      if (i == skip) continue;
      float f1 = faceF(i, x1);
      if (f1 > 0.0) {
        float f0 = faceF(i, x);
        float fr = f0 / (f0 - f1);
        if (fr < best) { best = fr; bi = i; }
      }
    }
    skip = -1;

    // Shell wall crossed inside this step (printed gradient only).
    float sb = 2.0, j1 = j0;
    if (layered()) {
      j1 = layerOf(dot(x1, x1));
      if (j1 != j0) {
        j1 = j0 + sign(j1 - j0);
        float rb = layerR(max(j0, j1));
        float f0 = length(x) - rb, f1 = length(x1) - rb;
        sb = clamp(f0 / (f0 - f1), 0.0, 1.0);
      }
    }

    if (sb < best && sb <= 1.0) {
      vec3 xh = mix(x, x1, sb);
      vec3 Th = normalize(mix(T, T1, sb));
      vec3 N = normalize(xh);
      if (dot(N, Th) > 0.0) N = -N;            // facing the incoming ray
      float na = layerNL(j0, xh), nb = layerNL(j1, xh);
      vec3 Tr = refract(Th, N, na / nb);
      // Partial (Fresnel) reflection at every printed layer wall, sampled stochastically;
      // beyond the critical angle this is total internal reflection.
      bool bounce = dot(Tr, Tr) == 0.0 || rnd(gSeed) < fresnel(-dot(Th, N), na, nb);
      if (bounce) { T = reflect(Th, N); x = xh + N * 1e-5; }   // stay in j0
      else { T = normalize(Tr); x = xh - N * 1e-5; }
    } else if (bi >= 0) {
      vec3 xh = mix(x, x1, clamp(best, 0.0, 1.0));
      vec3 Th = normalize(mix(T, T1, best));
      float e = edgeDist(xh, bi);
      float led = ledEmit(e);
      L += att * led;
      float cov = smoothstep(LED_W, LED_W * 0.55, e);
      att *= (1.0 - cov) * FILM_R;
      if (att < 0.003 || ++bounces > MAX_BOUNCE) break;
      vec3 N = faceN(bi, xh);
      T = reflect(Th, N);
      x = xh - N * 1e-5;
      skip = bi;
    } else {
      x = x1; T = T1;
    }
    if (uHole == 1 && dot(x, x) < HOLE_A * HOLE_A * 1.08) break; // captured
    if (dot(x, x) > RHO_V * RHO_V * 1.02) break;                  // numerical escape
  }
  return L;
}

// Seen from the surround: returns straight colour and coverage of the object.
vec4 shadeObject(vec3 ro, vec3 rd, vec3 tint) {
  float b = dot(ro, rd), c = dot(ro, ro) - RHO_V * RHO_V * 1.01;
  float disc = b * b - c;
  if (disc <= 0.0) return vec4(0.0);
  float tt = max(-b - sqrt(disc), 0.0), tEnd = -b + sqrt(disc);
  if (tEnd < 0.0) return vec4(0.0);
  int fi = 0; bool hit = false;
  for (int i = 0; i < 64; i++) {
    float f = objF(ro + rd * tt, fi);
    if (f < 1e-5) { hit = true; break; }
    tt += max(f, 2e-5);
    if (tt > tEnd) break;
  }
  if (!hit) return vec4(0.0);
  vec3 x = ro + rd * tt;
  vec3 N = faceN(fi, x);
  if (length(x) > RHO_V * 0.9995 && uCurved == 1) N = normalize(x);
  float cosi = clamp(-dot(rd, N), 0.0, 1.0);
  float n2 = nLambda(x);
  float F = fresnel(cosi, uNout, n2);
  float e = edgeDist(x, fi);
  float led = ledEmit(e);
  float cov = smoothstep(LED_W, LED_W * 0.55, e);
  vec3 T = refract(rd, N, uNout / n2);
  float inside = dot(T, T) > 0.0 ? traceInside(x - N * 1e-5, normalize(T), fi) : 0.0;
  vec3 refl = reflect(rd, N);
  // Studio surround: soft sky gradient, key softbox, and the plinth's glow from below.
  float env = 0.015 + 0.05 * smoothstep(-0.1, 1.0, refl.y)
            + 0.8 * pow(max(dot(refl, normalize(KEY)), 0.0), 120.0);
  float under = smoothstep(0.0, -0.8, refl.y) * 0.05;
  return vec4(tint * (inside * FILM_T * (1.0 - F) * (1.0 - cov) + led + under * F)
            + vec3(0.7, 0.85, 0.9) * env * (F + 0.25), 1.0);
}

void main() {
  uint seed = uint(gl_FragCoord.x) * 1973u + uint(gl_FragCoord.y) * 9277u + uFrame * 26699u;
  pcg(seed);
  vec2 jitter = vec2(rnd(seed), rnd(seed)) - 0.5;
  vec2 uv = ((gl_FragCoord.xy + jitter) * 2.0 - uRes) / uRes.y;
  vec3 rd = normalize(uCam * vec3(uv * uTanHalf, -1.0));

  float lambda = L0;
  vec3 tint = MINT;
  if (uWhite == 1) {
    lambda = 400.0 + 300.0 * rnd(seed);
    tint = spectrum(lambda) * SPEC_NORM * vec3(0.9, 0.95, 1.0);
  }
  float q = L0 / lambda;
  gDs = DISP * (q * q - 1.0);

  vec3 col = vec3(0.0);
  float alpha = 1.0;

  gSeed = seed;
  gSigma = ABSORB * (1.0 + 0.9 * pow(450.0 / lambda, 4.0));

  if (uInside == 1) {
    col = tint * traceInside(uRo, rd, -1);
  } else {
    vec4 obj = shadeObject(uRo, rd, tint);
    col = obj.rgb; alpha = obj.a;
    // Plinth: a dark gloss surface that catches the object's glow and mirrors it.
    float tf = (FLOOR_Y - uRo.y) / rd.y;
    if (obj.a == 0.0 && rd.y < 0.0 && uRo.y > FLOOR_Y && tf > 0.0) {
      vec3 p = uRo + rd * tf;
      float r2 = dot(p.xz, p.xz);
      float fade = exp(-r2 * 1.6);
      // LED light pooling under the object (inverse-square, cosine-weighted).
      float hgt = -FLOOR_Y;
      float d2 = r2 + hgt * hgt;
      float pool = 0.055 * hgt / (d2 * sqrt(d2));
      // Glossy reflection: jittered mirror direction, resolved by accumulation.
      vec3 rr = reflect(rd, vec3(0.0, 1.0, 0.0));
      rr = normalize(rr + (vec3(rnd(gSeed), rnd(gSeed), rnd(gSeed)) - 0.5) * 0.045);
      vec4 m = shadeObject(p + rr * 1e-4, rr, tint);
      float fr = 0.03 + 0.5 * pow(1.0 - clamp(-rd.y, 0.0, 1.0), 5.0);
      col = (tint * pool + m.rgb * (0.16 + fr)) * fade;
      alpha = fade * (0.25 + 0.75 * clamp(pool * 4.0 + m.a, 0.0, 1.0));
      col /= max(alpha, 1e-4);   // stored as straight colour below
    }
  }
  outColor = vec4(col * alpha, alpha);
}`;

const SHOW_FS = `#version 300 es
precision highp float;
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uAcc;
uniform float uExposure;
uniform vec2 uScale;   // fraction of the buffer holding the current (preview) image
uniform float uBloom;
void main() {
  vec2 uv = vUv * uScale;
  vec4 a = texture(uAcc, uv);
  vec4 s = a / max(uExposure, 1.0);
  vec3 c = s.rgb;
  // Lens bloom: a wide, soft glare around the LEDs from the mip chain
  // (a sum of progressively blurred copies approximates a long-tailed PSF).
  vec3 glare = vec3(0.0);
  float wsum = 0.0;
  for (int i = 0; i < 5; i++) {
    float lod = 1.5 + float(i) * 1.1;
    float wt = exp(-float(i) * 0.35);
    glare += textureLod(uAcc, uv, lod).rgb * wt;
    wsum += wt;
  }
  glare /= wsum * max(uExposure, 1.0);
  c += max(glare - 0.02, 0.0) * uBloom;
  c = 1.0 - exp(-c * 1.1);
  c = pow(c, vec3(0.92));
  float al = clamp(s.a, 0.0, 1.0);
  al = max(al, max(c.r, max(c.g, c.b)));
  outColor = vec4(c, al);
}`;

export function createInstrument(canvas) {
  const gl = getGL(canvas);
  if (!gl.getExtension('EXT_color_buffer_float')) throw new Error('Float render targets unavailable');

  const trace = program(gl, FULLSCREEN_VS, TRACE_FS);
  const show = program(gl, FULLSCREEN_VS, SHOW_FS);
  const vao = emptyVAO(gl);

  gl.useProgram(trace.program);
  gl.uniform3fv(trace.u.uU, flat(FACES));

  const state = { medium: 1, hole: 0, curved: 1, white: 0, inside: 0, nout: 1.0, steps: 0 };
  // Full-float accumulation keeps 400 additive samples precise; fall back to half floats.
  // (32F also needs linear filtering for the bloom's mip chain.)
  const full = gl.getExtension('EXT_float_blend') && gl.getExtension('OES_texture_float_linear');
  const accFormat = full ? gl.RGBA32F : gl.RGBA16F;

  let acc = null, fbo = null, w = 0, h = 0, frames = 0, levels = 1;
  function alloc() {
    if (acc) gl.deleteTexture(acc);
    acc = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, acc);
    levels = Math.floor(Math.log2(Math.max(w, h))) + 1;
    gl.texStorage2D(gl.TEXTURE_2D, levels, accFormat, w, h);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    if (!fbo) fbo = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, acc, 0);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    frames = 0;
  }

  const ctl = orbitControl(canvas, { yaw: 0.55, pitch: 0.32, autoSpin: 0 });
  canvas.style.cursor = 'grab';
  let lastView = '';
  let dirty = true;
  let still = 0;     // seconds since the view last changed
  let scale = 1;     // render scale: half resolution while the view is moving
  const MAX_FRAMES = 400;

  const loop = runWhenVisible(canvas, (time, dt) => {
    ctl.update(dt);
    fitCanvas(canvas, 1.1e6, 1.5);
    if (canvas.width !== w || canvas.height !== h || !acc) {
      w = canvas.width; h = canvas.height; alloc();
    }
    const view = `${ctl.yaw.toFixed(5)}|${ctl.pitch.toFixed(5)}`;
    const moved = view !== lastView;
    still = moved || ctl.dragging ? 0 : still + dt;
    const nextScale = still < 0.12 ? 0.5 : 1;
    if (moved || dirty || nextScale !== scale) { frames = 0; lastView = view; dirty = false; scale = nextScale; }
    const sw = Math.max(1, Math.round(w * scale)), sh = Math.max(1, Math.round(h * scale));

    gl.bindVertexArray(vao);
    if (frames < MAX_FRAMES) {
      gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
      gl.viewport(0, 0, sw, sh);
      if (frames === 0) { gl.clearColor(0, 0, 0, 0); gl.clear(gl.COLOR_BUFFER_BIT); }
      gl.enable(gl.BLEND);
      gl.blendFunc(gl.ONE, gl.ONE);
      gl.useProgram(trace.program);
      const cy = Math.cos(ctl.yaw), sy = Math.sin(ctl.yaw), cp = Math.cos(ctl.pitch), sp = Math.sin(ctl.pitch);
      const back = [sy * cp, sp, cy * cp], right = [cy, 0, -sy], up = [-sy * sp, cp, -cy * sp];
      gl.uniformMatrix3fv(trace.u.uCam, false, new Float32Array([...right, ...up, ...back]));
      const dist = state.inside ? 0.2 : 2.35;
      gl.uniform3f(trace.u.uRo, back[0] * dist, back[1] * dist, back[2] * dist);
      gl.uniform1f(trace.u.uTanHalf, Math.tan(((state.inside ? 82 : 30) * Math.PI) / 360));
      gl.uniform2f(trace.u.uRes, sw, sh);
      gl.uniform1ui(trace.u.uFrame, (frames + Math.floor(time * 1000)) >>> 0);
      gl.uniform1i(trace.u.uMedium, state.medium);
      gl.uniform1i(trace.u.uHole, state.hole);
      gl.uniform1i(trace.u.uCurved, state.curved);
      gl.uniform1i(trace.u.uWhite, state.white);
      gl.uniform1i(trace.u.uInside, state.inside);
      gl.uniform1f(trace.u.uNout, state.nout);
      gl.uniform1i(trace.u.uSteps, state.steps);
      gl.drawArrays(gl.TRIANGLES, 0, 3);
      gl.bindFramebuffer(gl.FRAMEBUFFER, null);
      frames++;
      gl.bindTexture(gl.TEXTURE_2D, acc);
      gl.generateMipmap(gl.TEXTURE_2D);
    }

    gl.viewport(0, 0, w, h);
    gl.disable(gl.BLEND);
    gl.clearColor(0, 0, 0, 0);
    gl.clear(gl.COLOR_BUFFER_BIT);
    gl.useProgram(show.program);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, acc);
    gl.uniform1i(show.u.uAcc, 0);
    gl.uniform1f(show.u.uExposure, frames);
    gl.uniform2f(show.u.uScale, sw / w, sh / h);
    gl.uniform1f(show.u.uBloom, state.inside ? 0.35 : 0.55);
    gl.drawArrays(gl.TRIANGLES, 0, 3);

    // Converged and at rest: stop the loop until something changes.
    if (frames >= MAX_FRAMES && !ctl.dragging) return false;
  });
  const wake = () => loop.kick();
  canvas.addEventListener('pointerdown', wake);
  canvas.addEventListener('pointermove', wake);
  new ResizeObserver(wake).observe(canvas);

  return {
    set(key, value) {
      state[key] = value;
      dirty = true;
      loop.kick();
    },
    get frames() { return frames; },
    get converged() { return frames >= MAX_FRAMES; },
    get maxFrames() { return MAX_FRAMES; },
  };
}
