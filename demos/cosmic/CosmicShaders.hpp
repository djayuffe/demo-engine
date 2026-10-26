#pragma once

// Native GPU interpretations of the user-supplied Möbius universe and advanced
// quantum-fractal browser studies. Geometry is generated from gl_VertexID so
// animation never rebuilds or uploads vertex buffers.
namespace cosmic_gpu {
inline constexpr int mobiusU = 320;
inline constexpr int mobiusV = 80;
inline constexpr int mobiusSurfaceVertices = mobiusU * mobiusV * 6;
inline constexpr int mobiusCorePoints = 8192;
inline constexpr int mobiusHorizonPoints = 4096;
inline constexpr int mobiusStarPoints = 12288;
inline constexpr int mobiusDebrisPoints = 6144;
inline constexpr int mobiusFlowPoints = 24576;
inline constexpr int mobiusPointCount = mobiusCorePoints + mobiusHorizonPoints + mobiusStarPoints +
                                        mobiusDebrisPoints + mobiusFlowPoints;

inline constexpr int quantumFieldPoints = 98304;
inline constexpr int quantumBraidPoints = 8192;
inline constexpr int quantumNodePoints = 4096;
inline constexpr int quantumStarPoints = 8192;
inline constexpr int quantumPointCount = quantumFieldPoints + quantumBraidPoints + quantumNodePoints +
                                         quantumStarPoints;
inline constexpr int hyperFieldPoints = 65536;
inline constexpr int hyperScaffoldPoints = 16384;
inline constexpr int hyperStarPoints = 8192;
inline constexpr int hyperPointCount = hyperFieldPoints + hyperScaffoldPoints + hyperStarPoints;
inline constexpr int blackHoleDiskParticles = 98304;
inline constexpr int blackHoleJetParticles = 32768;
inline constexpr int blackHoleParticleCount = blackHoleDiskParticles + blackHoleJetParticles;

inline constexpr const char* mobiusSurfaceVertex = R"COSMIC(#version 410 core
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec2 uRes;
uniform float uTime, uPulse, uBass, uMid, uTreble;
out vec3 vNormal;
out vec3 vView;
out vec3 vBase;
out vec2 vParam;
out float vFlow;
out float vDepth;

const float PI = 3.141592653589793;
const int U_CELLS = 320;
const int V_CELLS = 80;

// Analytic derivatives share the position's trigonometry instead of evaluating
// four neighbouring surfaces per vertex. Includes the animated scale derivative.
struct MobiusSample { vec3 position; vec3 du; vec3 dv; };
MobiusSample sampleMobius(float u, float v, float time, float bass) {
  float ribbonAngle = u * 0.5 + sin(u * 0.5) * 2.5;
  float ribbonDerivative = 0.5 + 1.25 * cos(u * 0.5);
  float su = sin(u), cu = cos(u), sa = sin(ribbonAngle), ca = cos(ribbonAngle);
  float phaseU = u * 20.0 + time * 0.7, phaseV = v * 15.0 - time * 0.31;
  float scale = 0.82 * (1.0 + 0.018 * sin(phaseU) + 0.016 * cos(phaseV) + 0.018 * bass);
  float radius = 1.5 + 0.5 * v * ca;
  float radiusU = -0.5 * v * sa * ribbonDerivative;
  vec3 base = vec3(radius * cu, radius * su, 0.5 * v * sa + 0.25 * cos(u * 0.25));
  vec3 baseU = vec3(radiusU * cu - radius * su, radiusU * su + radius * cu,
                    0.5 * v * ca * ribbonDerivative - 0.0625 * sin(u * 0.25));
  vec3 baseV = vec3(0.5 * ca * cu, 0.5 * ca * su, 0.5 * sa);
  return MobiusSample(base * scale,
                      baseU * scale + base * (0.2952 * cos(phaseU)),
                      baseV * scale - base * (0.1968 * sin(phaseV)));
}
vec3 safeNormalize(vec3 v, vec3 fallback) {
  float m = dot(v, v);
  return m > 1e-10 ? v * inversesqrt(m) : fallback;
}
void main() {
  int triangleVertex = gl_VertexID % 6;
  int cell = gl_VertexID / 6;
  int ui = cell % U_CELLS;
  int vi = cell / U_CELLS;
  ivec2 corner;
  if (triangleVertex == 0) corner = ivec2(0, 0);
  else if (triangleVertex == 1) corner = ivec2(1, 0);
  else if (triangleVertex == 2) corner = ivec2(1, 1);
  else if (triangleVertex == 3) corner = ivec2(0, 0);
  else if (triangleVertex == 4) corner = ivec2(1, 1);
  else corner = ivec2(0, 1);
  float u = float(ui + corner.x) * (4.0 * PI / float(U_CELLS));
  float v = float(vi + corner.y) * (2.0 / float(V_CELLS)) - 1.0;
  float time = uTime + 0.12 * uPulse;
  MobiusSample surface = sampleMobius(u, v, time, uBass);
  vec3 p = surface.position;
  vNormal = safeNormalize(cross(surface.du, surface.dv), vec3(0.0, 0.0, 1.0));
  vView = safeNormalize(uEye - p, vec3(0.0, 0.0, 1.0));
  float flow = 0.5 + 0.5 * sin(u * 7.0 - uTime * (2.4 + 0.8 * uMid));
  float dimension = 0.5 + 0.5 * cos(u * 0.25 + v * PI);
  vBase = mix(vec3(0.025, 0.20, 0.58), vec3(0.76, 0.035, 0.34), dimension);
  vBase = mix(vBase, vec3(0.12, 0.86, 1.20), flow * (0.22 + 0.30 * uTreble));
  vParam = vec2(u, v);
  vFlow = flow;
  vec3 view = transpose(uCam) * (p - uEye);
  float depth = -view.z;
  vDepth = depth;
  float aspect = uRes.x / max(uRes.y, 1.0);
  vec2 ndc = view.xy / (max(depth, 0.05) * vec2(0.52 * aspect, 0.52));
  gl_Position = depth > 0.05 ? vec4(ndc, clamp(depth / 12.0, 0.0, 1.0) * 2.0 - 1.0, 1.0)
                            : vec4(2.0, 2.0, 2.0, 1.0);
}
)COSMIC";

inline constexpr const char* mobiusSurfaceFragment = R"COSMIC(#version 410 core
in vec3 vNormal;
in vec3 vView;
in vec3 vBase;
in vec2 vParam;
in float vFlow;
in float vDepth;
out vec4 outColor;
uniform float uPulse, uBass, uTreble;
vec3 safeNormalize(vec3 value, vec3 fallback) {
  float magnitudeSquared = dot(value, value);
  return magnitudeSquared > 1e-12 ? value * inversesqrt(magnitudeSquared) : fallback;
}
void main() {
  vec3 n = safeNormalize(vNormal, vec3(0.0, 0.0, 1.0));
  vec3 v = safeNormalize(vView, vec3(0.0, 0.0, 1.0));
  vec3 l = safeNormalize(vec3(-0.42, 0.74, 0.52), vec3(0.0, 1.0, 0.0));
  vec3 h = safeNormalize(l + v, l);
  float ndl = max(0.0, dot(n, l));
  float ndh = max(0.0, dot(n, h));
  float nv = abs(dot(n, v));
  float fresnel = pow(1.0 - nv, 3.0);
  float longitudinal = 0.5 + 0.5 * sin(vParam.x * 56.0 + sin(vParam.y * 17.0) * 1.8);
  float transverse = 0.5 + 0.5 * cos(vParam.y * 46.0 - vParam.x * 3.0);
  float weave = mix(longitudinal, transverse, 0.34);
  float filament = pow(max(longitudinal, transverse), 10.0);
  float roughness = mix(30.0, 86.0, weave);
  float metal = pow(ndh, roughness) * (0.45 + 0.55 * ndl);
  vec3 body = vBase * (0.10 + 0.36 * ndl + 0.13 * weave + 0.08 * filament);
  vec3 edge = mix(vec3(0.04, 0.62, 1.35), vec3(1.35, 0.12, 0.48), vFlow);
  vec3 emission = edge * (0.22 + 0.82 * fresnel + 0.46 * metal + 0.10 * filament * uTreble +
                          0.24 * uPulse + 0.12 * uTreble);
  float fog = smoothstep(5.0, 11.0, vDepth);
  vec3 color = mix(body + emission, vec3(0.012, 0.025, 0.09), fog * 0.45);
  outColor = vec4(color * (0.72 + 0.18 * uBass), 1.0);
}
)COSMIC";

inline constexpr const char* mobiusPointVertex = R"COSMIC(#version 410 core
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec2 uRes;
uniform float uTime, uPulse, uBass, uMid, uTreble;
out vec3 vColor;
out float vOpacity;
out float vKind;
out float vDepth;

const float PI = 3.141592653589793;
const int CORE = 8192;
const int HORIZON = 4096;
const int STARS = 12288;
const int DEBRIS = 6144;
uint hash(uint x) { x ^= x >> 16u; x *= 0x7feb352du; x ^= x >> 15u; x *= 0x846ca68bu; return x ^ (x >> 16u); }
float random(uint x) { return float(hash(x)) * (1.0 / 4294967295.0); }
vec3 safeNormalize(vec3 value, vec3 fallback) {
  float magnitudeSquared = dot(value, value);
  return magnitudeSquared > 1e-12 ? value * inversesqrt(magnitudeSquared) : fallback;
}
vec3 sphere(int index, int count) {
  float q = (float(index) + 0.5) / float(count), z = 1.0 - 2.0 * q;
  float a = float(index) * 2.399963229728653, r = sqrt(max(0.0, 1.0 - z * z));
  return vec3(r * cos(a), z, r * sin(a));
}
vec3 mobius(float u, float v) {
  float twist = sin(u * 0.5) * 2.5, a = u * 0.5 + twist;
  float r = 1.5 + 0.5 * v * cos(a);
  return vec3(r * cos(u), r * sin(u), 0.5 * v * sin(a) + 0.25 * cos(u * 0.25)) * 0.82;
}
void main() {
  int id = gl_VertexID;
  vec3 p;
  float size = 2.0;
  vOpacity = 1.0;
  if (id < CORE) {
    vec3 n = sphere(id, CORE);
    p = n * (0.31 + 0.035 * uBass);
    float facing = 1.0 - clamp(abs(dot(n, safeNormalize(uEye - p, vec3(0.0, 0.0, 1.0)))), 0.0, 1.0);
    float rim = facing * facing;
    vColor = vec3(0.025, 0.055, 0.12) + vec3(0.20, 0.82, 1.65) * rim + vec3(1.4, 0.16, 0.30) * uPulse;
    vKind = 0.0; size = 3.1 + 1.1 * uPulse;
  } else if (id < CORE + HORIZON) {
    vec3 n = sphere(id - CORE, HORIZON);
    p = n * (0.48 + 0.045 * uPulse);
    vColor = mix(vec3(0.04, 0.30, 1.25), vec3(1.25, 0.05, 0.34), 0.5 + 0.5 * n.y) * (0.58 + 0.48 * uBass);
    vOpacity = 0.62; vKind = 1.0; size = 2.4 + 1.3 * uPulse;
  } else if (id < CORE + HORIZON + STARS) {
    uint sid = uint(id - CORE - HORIZON);
    float a = random(sid * 3u) * 2.0 * PI, z = random(sid * 3u + 1u) * 2.0 - 1.0;
    float r = 5.0 + 5.0 * random(sid * 3u + 2u);
    p = vec3(sqrt(max(0.0, 1.0 - z * z)) * cos(a), z, sqrt(max(0.0, 1.0 - z * z)) * sin(a)) * r;
    vColor = mix(vec3(0.16, 0.42, 1.35), vec3(1.34, 0.66, 0.24), random(sid * 9u)) * (0.35 + uTreble);
    vOpacity = 0.7; vKind = 2.0; size = 1.4 + 1.8 * uTreble;
  } else if (id < CORE + HORIZON + STARS + DEBRIS) {
    uint did = uint(id - CORE - HORIZON - STARS);
    float a = random(did * 4u) * 2.0 * PI + uTime * (0.03 + 0.03 * uMid);
    float r = 2.1 + 1.8 * random(did * 4u + 1u);
    p = vec3(cos(a) * r, (random(did * 4u + 2u) - 0.5) * 1.3, sin(a) * r);
    vColor = mix(vec3(0.08, 0.24, 0.55), vec3(0.72, 0.16, 0.48), random(did * 4u + 3u));
    vOpacity = 0.52; vKind = 3.0; size = 1.8 + 1.0 * uBass;
  } else {
    uint fid = uint(id - CORE - HORIZON - STARS - DEBRIS);
    float u = random(fid * 2u) * 4.0 * PI + uTime * (0.08 + 0.04 * uMid);
    float v = random(fid * 2u + 1u) * 2.0 - 1.0;
    p = mobius(u, v);
    float stream = 0.5 + 0.5 * sin(u * 6.0 - uTime * 3.0);
    vColor = mix(vec3(0.06, 0.62, 1.5), vec3(1.55, 0.12, 0.43), stream) * (0.42 + 0.44 * uMid + 0.35 * uPulse);
    vOpacity = 0.48 + 0.42 * stream; vKind = 4.0; size = 1.5 + 1.5 * stream + uPulse;
  }
  vec3 view = transpose(uCam) * (p - uEye);
  float depth = -view.z;
  vDepth = depth;
  float aspect = uRes.x / max(uRes.y, 1.0);
  gl_Position = depth > 0.05 ? vec4(view.xy / (depth * vec2(0.52 * aspect, 0.52)),
                                    clamp(depth / 12.0, 0.0, 1.0) * 2.0 - 1.0, 1.0)
                            : vec4(2.0, 2.0, 2.0, 1.0);
  gl_PointSize = depth > 0.05 ? clamp(size * uRes.y / (500.0 * depth), 1.0, 10.0) : 1.0;
}
)COSMIC";

inline constexpr const char* pointFragment = R"COSMIC(#version 410 core
in vec3 vColor;
in float vOpacity;
in float vKind;
in float vDepth;
out vec4 outColor;
void main() {
  vec2 p = gl_PointCoord * 2.0 - 1.0;
  float r2 = dot(p, p);
  if (r2 > 1.0) discard;
  float z = sqrt(max(0.0, 1.0 - r2));
  float edgeWidth = max(fwidth(r2), 0.002);
  float halo = 1.0 - smoothstep(0.08, 1.0, r2);
  float core = 1.0 - smoothstep(0.0, 0.14, r2);
  float flare = pow(max(0.0, 1.0 - abs(p.x)), 18.0) + pow(max(0.0, 1.0 - abs(p.y)), 18.0);
  float spec = pow(max(0.0, dot(normalize(vec3(-0.4, 0.7, 0.65)), vec3(p, z))), 18.0);
  float azimuth = atan(p.y, p.x);
  float facets = 0.78 + 0.22 * cos(azimuth * (vKind < 1.5 ? 8.0 : 6.0));
  float shell = 1.0 - smoothstep(edgeWidth * 2.0, edgeWidth * 7.0, abs(r2 - 0.68));
  float energy = vKind < 0.5 ? 0.28 * halo + 0.72 * spec + core :
                 vKind < 1.5 ? (0.34 * halo + 0.42 * core + 0.30 * (1.0 - z) + 0.13 * shell) * facets :
                 vKind < 2.5 ? 0.18 * halo + 0.86 * core + 0.12 * flare :
                 vKind < 3.5 ? (0.42 * halo + 0.30 * spec) * facets :
                 vKind < 4.5 ? 0.25 * halo + 0.74 * core : 0.10 * halo + 0.34 * core;
  float fog = smoothstep(6.0, 12.0, vDepth);
  vec3 color = mix(vColor, vec3(0.012, 0.025, 0.085), fog * 0.48);
  outColor = vec4(color * energy * vOpacity, halo * vOpacity);
}
)COSMIC";

// A zero-upload particle pass for the black hole. Every orbit and jet path is
// reconstructed from gl_VertexID, leaving animation and physics on the GPU.
inline constexpr const char* blackHoleParticleVertex = R"COSMIC(#version 410 core
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec2 uRes;
uniform float uTime, uPulse, uBass, uMid, uTreble;
out vec3 vColor;
out float vOpacity;
out float vKind;
out float vDepth;

const float PI = 3.141592653589793;
const int DISK_PARTICLES = 98304;
uint hash(uint x) { x ^= x >> 16u; x *= 0x7feb352du; x ^= x >> 15u; x *= 0x846ca68bu; return x ^ (x >> 16u); }
float random(uint x) { return float(hash(x)) * (1.0 / 4294967295.0); }
vec3 safeNormalize(vec3 value, vec3 fallback) {
  float magnitudeSquared = dot(value, value);
  return magnitudeSquared > 1e-12 ? value * inversesqrt(magnitudeSquared) : fallback;
}
void main() {
  int id = gl_VertexID;
  uint seed = uint(id) * 7u + 19u;
  vec3 diskNormal = normalize(vec3(0.22, 0.86, 0.46));
  vec3 diskX = normalize(cross(diskNormal, vec3(0.0, 1.0, 0.0)));
  vec3 diskZ = cross(diskNormal, diskX);
  vec3 p;
  float size;

  if (id < DISK_PARTICLES) {
    // A long-lived Lagrangian dust parcel. Conserved angular momentum gives
    // the visible Kepler speed-up as its orbit spirals toward the photon ring.
    float birthRadius = mix(1.20, 4.70, pow(random(seed), 0.72));
    float lifetime = mix(7.0, 18.0, random(seed + 1u));
    float age = fract(random(seed + 2u) + uTime / lifetime);
    float inwardDrift = age * age * mix(0.30, 1.36, random(seed + 3u));
    float radius = max(0.88, birthRadius - inwardDrift);
    float keplerOmega = (0.82 + 0.16 * uBass) * inversesqrt(birthRadius * birthRadius * birthRadius);
    float angle = random(seed + 4u) * 2.0 * PI + uTime * keplerOmega + age * age * 3.8;
    float epicycle = sin(angle * 3.0 + random(seed + 5u) * 2.0 * PI) * (0.012 + 0.035 * radius);
    float scaleHeight = mix(0.012, 0.105, smoothstep(1.0, 4.7, radius));
    float vertical = (random(seed + 6u) * 2.0 - 1.0) * scaleHeight;
    vertical += sin(angle * 2.0 + random(seed + 7u) * 6.0) * scaleHeight * 0.28;
    p = diskX * (cos(angle) * (radius + epicycle)) +
        diskZ * (sin(angle) * (radius + epicycle)) + diskNormal * vertical;

    vec3 orbitalVelocity = safeNormalize(diskX * -sin(angle) + diskZ * cos(angle), diskX);
    vec3 viewDirection = safeNormalize(uEye - p, diskZ);
    float lineVelocity = clamp(dot(orbitalVelocity, viewDirection), -1.0, 1.0);
    float heat = 1.0 - smoothstep(0.88, 4.45, radius);
    vec3 ember = mix(vec3(0.72, 0.035, 0.006), vec3(2.10, 0.72, 0.10), heat);
    ember = mix(ember, lineVelocity > 0.0 ? vec3(0.55, 0.92, 2.35) : vec3(2.20, 0.10, 0.018),
                abs(lineVelocity) * 0.26);
    float filament = 0.52 + 0.48 * pow(0.5 + 0.5 * sin(angle * 13.0 - radius * 17.0), 8.0);
    vColor = ember * filament * (0.46 + 0.38 * uBass + 0.42 * uPulse);
    vOpacity = (0.20 + 0.54 * heat) * smoothstep(0.88, 0.96, radius);
    vKind = 4.0;
    size = mix(1.15, 2.75, heat) + 1.25 * uPulse;
  } else {
    // Bipolar relativistic ejecta: ballistic axial acceleration wrapped in a
    // widening magnetic helix. Each parcel fades before it is recycled.
    uint jetId = uint(id - DISK_PARTICLES);
    float signAxis = (jetId & 1u) == 0u ? -1.0 : 1.0;
    float lifetime = mix(4.0, 9.0, random(seed + 1u));
    float age = fract(random(seed + 2u) + uTime / lifetime);
    float axialDistance = 0.72 + 7.4 * pow(age, 1.42);
    float helixAngle = random(seed + 3u) * 2.0 * PI + signAxis * (age * 25.0 - uTime * 1.8);
    float opening = (0.025 + 0.21 * age) * (0.45 + random(seed + 4u));
    float shock = sin(age * 54.0 - uTime * 4.2);
    p = diskNormal * (signAxis * axialDistance) +
        diskX * (cos(helixAngle) * opening) + diskZ * (sin(helixAngle) * opening);
    p += diskNormal * (signAxis * shock * 0.035 * (0.4 + uTreble));
    float fade = smoothstep(0.0, 0.055, age) * (1.0 - smoothstep(0.70, 1.0, age));
    float knot = 0.55 + 0.45 * pow(0.5 + 0.5 * shock, 10.0);
    vColor = mix(vec3(0.10, 0.50, 2.10), vec3(1.25, 0.20, 1.80), random(seed + 5u));
    vColor *= knot * (0.46 + 0.62 * uTreble + 0.52 * uPulse);
    vOpacity = fade * (0.24 + 0.42 * knot);
    vKind = 2.0;
    size = 1.1 + 2.0 * knot + 1.4 * uPulse;
  }

  vec3 view = transpose(uCam) * (p - uEye);
  vec3 centerView = transpose(uCam) * -uEye;
  float depth = -view.z;
  float centerDepth = -centerView.z;
  // Event-horizon occultation keeps far-side dust from drawing through the
  // shadow even though this additive pass intentionally has no depth buffer.
  if (depth > centerDepth && length(view.xy - centerView.xy) < 0.74) vOpacity = 0.0;
  vDepth = depth;
  float aspect = uRes.x / max(uRes.y, 1.0);
  gl_Position = depth > 0.05 ? vec4(view.xy / (depth * vec2(0.52 * aspect, 0.52)),
                                    clamp(depth / 14.0, 0.0, 1.0) * 2.0 - 1.0, 1.0)
                            : vec4(2.0, 2.0, 2.0, 1.0);
  gl_PointSize = depth > 0.05 ? clamp(size * uRes.y / (430.0 * depth), 1.0, 11.0) : 1.0;
}
)COSMIC";

inline constexpr const char* quantumVertex = R"COSMIC(#version 410 core
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec2 uRes;
uniform float uTime, uPulse, uBass, uMid, uTreble;
out vec3 vColor;
out float vOpacity;
out float vKind;
out float vDepth;

const float PI = 3.141592653589793;
const int FIELD = 98304;
const int BRAIDS = 8192;
const int NODES = 4096;
uint hash(uint x) { x ^= x >> 16u; x *= 0x7feb352du; x ^= x >> 15u; x *= 0x846ca68bu; return x ^ (x >> 16u); }
float random(uint x) { return float(hash(x)) * (1.0 / 4294967295.0); }
vec3 sphere(int index, int count) {
  float q = (float(index) + 0.5) / float(count), z = 1.0 - 2.0 * q;
  float a = float(index) * 2.399963229728653, r = sqrt(max(0.0, 1.0 - z * z));
  return vec3(r * cos(a), z, r * sin(a));
}
vec3 rotateY(vec3 p, float a) { float c = cos(a), s = sin(a); return vec3(c*p.x+s*p.z, p.y, -s*p.x+c*p.z); }
vec3 hsv(float h, float s, float v) {
  vec3 p = abs(fract(vec3(h) + vec3(0.0, 2.0/3.0, 1.0/3.0)) * 6.0 - 3.0);
  return v * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), s);
}
void main() {
  int id = gl_VertexID;
  vec3 p;
  float size = 2.0;
  vOpacity = 1.0;
  if (id < FIELD) {
    uint sid = uint(id);
    float echo = float(gl_InstanceID);
    float u = random(sid * 2u) * 2.0 * PI;
    float v = random(sid * 2u + 1u) * PI;
    float delay = echo * (0.045 + 0.025 * uBass);
    float phase = uTime * 0.52 - delay + 0.18 * uPulse;
    float interference = sin(u * 0.5 + v + phase);
    float y = sin(u) * sin(v) * interference;
    float indexPhase = float(id) * 0.01745329252;
    p = vec3(cos(u) * sin(v) + sin(indexPhase),
             sin(u) * cos(v) * interference + cos(indexPhase), y) * (0.72 + 0.035 * uBass);
    p = rotateY(p, uTime * (0.075 + 0.012 * uMid) - delay);
    float charge = 0.5 + 0.5 * sin(4.0 * y + uTime * 0.42);
    vColor = hsv(fract(0.52 + 0.34 * charge + 0.025 * echo), 0.82,
                 (0.72 + 0.40 * uTreble + 0.44 * uPulse) * exp(-echo * 0.38));
    vOpacity = exp(-echo * 0.52);
    vKind = 5.0;
    size = 1.45 + 1.30 * uPulse + 0.16 * echo;
  } else if (id < FIELD + BRAIDS) {
    int bid = id - FIELD;
    int strand = bid % 8;
    float t = float(bid / 8) / 1023.0;
    float z = mix(-2.5, 2.5, t);
    float a = z * (3.2 + 0.35 * uMid) + float(strand) * PI * 0.25 - uTime * 0.7;
    float radius = 0.42 + 0.12 * sin(z * 2.4 + float(strand));
    p = vec3(cos(a) * radius, sin(a) * radius, z);
    p = rotateY(p, PI * 0.5 + 0.16 * sin(uTime * 0.2));
    vColor = mix(vec3(0.08, 0.72, 1.55), vec3(1.58, 0.12, 0.38), mod(float(strand), 2.0)) * (0.55 + 0.55 * uMid + 0.35 * uPulse);
    vOpacity = 0.82; vKind = 1.0; size = 2.0 + 1.2 * uPulse;
  } else if (id < FIELD + BRAIDS + NODES) {
    int nid = id - FIELD - BRAIDS;
    int node = nid / 512;
    vec3 n = sphere(nid % 512, 512);
    float a = float(node) * PI * 0.25 + uTime * 0.11;
    vec3 center = vec3(cos(a) * 1.72, 0.36 * sin(a * 2.0), sin(a) * 1.72);
    p = center + n * (0.105 + 0.025 * uBass);
    vColor = mix(vec3(0.10, 0.48, 1.42), vec3(1.48, 0.12, 0.55), mod(float(node), 2.0)) * (0.72 + 0.65 * uPulse);
    vKind = 0.0; size = 3.0;
  } else {
    uint sid = uint(id - FIELD - BRAIDS - NODES);
    float a = random(sid * 3u) * 2.0 * PI, z = random(sid * 3u + 1u) * 2.0 - 1.0;
    float r = 5.5 + 3.0 * random(sid * 3u + 2u);
    p = vec3(sqrt(max(0.0, 1.0-z*z))*cos(a), z, sqrt(max(0.0, 1.0-z*z))*sin(a)) * r;
    vColor = mix(vec3(0.12, 0.38, 1.25), vec3(1.25, 0.72, 0.30), random(sid * 5u)) * (0.30 + uTreble);
    vOpacity = 0.72; vKind = 2.0; size = 1.4 + 1.6 * uTreble;
  }
  vec3 view = transpose(uCam) * (p - uEye);
  float depth = -view.z;
  vDepth = depth;
  float aspect = uRes.x / max(uRes.y, 1.0);
  gl_Position = depth > 0.05 ? vec4(view.xy / (depth * vec2(0.52 * aspect, 0.52)),
                                    clamp(depth / 12.0, 0.0, 1.0) * 2.0 - 1.0, 1.0)
                            : vec4(2.0, 2.0, 2.0, 1.0);
  gl_PointSize = depth > 0.05 ? clamp(size * uRes.y / (500.0 * depth), 1.0, 10.0) : 1.0;
}
)COSMIC";

inline constexpr const char* blackHoleFragment = R"COSMIC(#version 410 core
in vec2 vUv;
out vec4 outColor;
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec2 uRes;
uniform float uTime, uPulse, uBass, uMid, uTreble;

const float PI = 3.141592653589793;
vec3 safeNormalize(vec3 value, vec3 fallback) {
  float magnitudeSquared = dot(value, value);
  return magnitudeSquared > 1e-12 ? value * inversesqrt(magnitudeSquared) : fallback;
}
float hash12(vec2 p) {
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}
float noise(vec3 p) {
  vec3 i = floor(p), f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  float n = dot(i, vec3(1.0, 57.0, 113.0));
  return mix(mix(mix(hash12(vec2(n, n + 1.0)), hash12(vec2(n + 1.0, n + 2.0)), f.x),
                 mix(hash12(vec2(n + 57.0, n + 58.0)), hash12(vec2(n + 58.0, n + 59.0)), f.x), f.y),
             mix(mix(hash12(vec2(n + 113.0, n + 114.0)), hash12(vec2(n + 114.0, n + 115.0)), f.x),
                 mix(hash12(vec2(n + 170.0, n + 171.0)), hash12(vec2(n + 171.0, n + 172.0)), f.x), f.y), f.z);
}
float fbm(vec3 p) {
  float sum = 0.0, amplitude = 0.5;
  for (int i = 0; i < 5; ++i) {
    sum += amplitude * noise(p);
    p = p * 2.03 + vec3(1.7, 9.2, 2.4);
    amplitude *= 0.5;
  }
  return sum;
}
vec3 starLayer(vec2 spherical, float scale, float threshold) {
  vec2 grid = spherical * scale;
  vec2 cell = floor(grid);
  vec2 local = fract(grid) - 0.5;
  float seed = hash12(cell);
  vec2 center = vec2(hash12(cell + 17.0), hash12(cell + 43.0)) - 0.5;
  center *= 0.56;
  float distanceToStar = length(local - center);
  float radius = mix(0.018, 0.052, hash12(cell + 71.0));
  float coverage = 1.0 - smoothstep(radius, radius + 0.018, distanceToStar);
  float occupancy = smoothstep(threshold, 1.0, seed);
  float core = coverage * occupancy * mix(0.35, 1.35, hash12(cell + 91.0));
  vec3 temperature = mix(vec3(0.22, 0.46, 1.1), vec3(1.2, 0.68, 0.30), hash12(cell + 113.0));
  return temperature * core;
}
vec3 stars(vec3 rd) {
  vec2 spherical = vec2(atan(rd.z, rd.x) / (2.0 * PI), asin(clamp(rd.y, -1.0, 1.0)) / PI);
  vec3 field = starLayer(spherical, 310.0, 0.945);
  field += starLayer(spherical + vec2(0.173, 0.319), 570.0, 0.978) * 0.72;
  return field * (0.42 + 0.82 * uTreble);
}
void main() {
  vec2 p = vUv * 2.0 - 1.0;
  p.x *= uRes.x / max(uRes.y, 1.0);
  // uEye is already a world-space camera translation. Applying uCam to it a
  // second time rotated parallax incorrectly as the view orbited.
  vec3 ro = uCam * vec3(0.0, 0.0, 5.2) + uEye * 0.72;
  vec3 rd = safeNormalize(uCam * vec3(p * 0.72, -1.5), vec3(0.0, 0.0, -1.0));
  vec3 pos = ro;
  vec3 dir = rd;
  vec3 color = vec3(0.0);
  float transmission = 1.0;
  float closest = 100.0;
  vec3 diskNormal = normalize(vec3(0.22, 0.86, 0.46));
  vec3 diskX = normalize(cross(diskNormal, vec3(0.0, 1.0, 0.0)));
  vec3 diskZ = cross(diskNormal, diskX);
  float previousPlane = dot(pos, diskNormal);
  bool swallowed = false;
  for (int stepIndex = 0; stepIndex < 96; ++stepIndex) {
    float radius = max(length(pos), 0.001);
    closest = min(closest, radius);
    if (radius < 0.72) {
      swallowed = true;
      transmission = 0.0;
      break;
    }
    float stepSize = clamp((radius - 0.68) * 0.075, 0.012, 0.105);
    // Integrate curvature per travelled distance so adaptive spatial steps do
    // not silently change gravity. The coefficient preserves the former
    // far-field deflection while damping numerical overshoot near the horizon.
    float gravity = (0.15 + 0.05 * uBass) * stepSize / max(radius * radius * radius, 0.18);
    dir = safeNormalize(dir - pos * gravity, dir);
    vec3 nextPos = pos + dir * stepSize;
    float nextPlane = dot(nextPos, diskNormal);
    vec3 projectedDisk = nextPos - diskNormal * nextPlane;
    float projectedRadius = length(projectedDisk);
    if (projectedRadius > 1.02 && projectedRadius < 4.35 && abs(nextPlane) < 0.22) {
      float volumeAngle = atan(dot(projectedDisk, diskZ), dot(projectedDisk, diskX));
      float radialDensity = smoothstep(1.02, 1.28, projectedRadius) *
                            (1.0 - smoothstep(3.45, 4.35, projectedRadius));
      float diskHeight = nextPlane / mix(0.055, 0.16, projectedRadius / 4.35);
      float verticalDensity = exp(-diskHeight * diskHeight);
      float filaments = 0.42 + 0.58 * pow(0.5 + 0.5 * sin(volumeAngle * 11.0 - projectedRadius * 18.0 -
                                                          uTime * (1.35 + 0.25 * uMid)), 6.0);
      float density = radialDensity * verticalDensity * filaments;
      float heat = clamp((4.35 - projectedRadius) / 3.3, 0.0, 1.0);
      vec3 volumeColor = mix(vec3(0.48, 0.025, 0.008), vec3(1.32, 0.34, 0.055), heat);
      // Homogeneous-segment emission/absorption. Fourth-order evaluation
      // avoids cancellation in 1-exp(-tau) for thin samples (tau <= 0.036).
      float tau = density * stepSize * 0.34;
      float opacity = tau * (1.0 - tau * (0.5 - tau * (1.0 / 6.0 - tau / 24.0)));
      color += transmission * volumeColor * opacity * ((0.16 + 0.12 * uBass) / 0.34);
      transmission *= 1.0 - opacity;
    }
    if (previousPlane * nextPlane <= 0.0) {
      float planeDelta = previousPlane - nextPlane;
      float crossing = abs(planeDelta) > 1e-6 ? clamp(previousPlane / planeDelta, 0.0, 1.0) : 0.0;
      vec3 crossingPosition = mix(pos, nextPos, crossing);
      vec3 diskPosition = crossingPosition - diskNormal * dot(crossingPosition, diskNormal);
      float diskRadius = length(diskPosition);
      if (diskRadius > 1.05 && diskRadius < 4.15) {
        float angle = atan(dot(diskPosition, diskZ), dot(diskPosition, diskX));
        float spiral = angle * 7.0 - log(diskRadius) * 13.0 - uTime * (1.1 + 0.25 * uMid);
        float turbulence = fbm(vec3(angle * 2.2, diskRadius * 2.6, uTime * 0.08));
        float bands = pow(0.5 + 0.5 * sin(spiral + turbulence * 5.0), 4.0);
        float radial = smoothstep(1.05, 1.35, diskRadius) * (1.0 - smoothstep(3.2, 4.15, diskRadius));
        float heat = clamp((4.15 - diskRadius) / 3.1, 0.0, 1.0);
        vec3 plasma = mix(vec3(0.85, 0.035, 0.008), vec3(1.75, 0.72, 0.16), heat);
        plasma = mix(plasma, vec3(0.48, 0.72, 1.55), pow(heat, 5.0) * 0.45);
        vec3 orbitalTangent = safeNormalize(cross(diskNormal, diskPosition), diskX);
        float lineVelocity = clamp(dot(orbitalTangent, -dir), -1.0, 1.0);
        float doppler = clamp(1.0 + lineVelocity * 0.46 / sqrt(max(diskRadius, 1.0)), 0.58, 1.48);
        plasma = mix(plasma, lineVelocity > 0.0 ? vec3(0.50, 0.78, 1.65) : vec3(1.55, 0.13, 0.025),
                     abs(lineVelocity) * 0.24);
        float wisps = 0.72 + 0.28 * fbm(vec3(angle * 8.0, diskRadius * 12.0, uTime * 0.22));
        float energy = radial * (0.025 + 0.28 * bands) * wisps * pow(doppler, 2.4) *
                       (0.55 + 0.45 * uBass + 0.25 * uPulse);
        color += transmission * plasma * energy;
        transmission *= 0.90;
      }
    }
    float jetAxis = dot(nextPos, diskNormal);
    // Reuse the disk projection instead of another vector projection/sqrt.
    float jet = exp(-projectedRadius * 10.0) * smoothstep(0.7, 2.8, abs(jetAxis));
    float knot = max(0.0, 0.45 + 0.55 * sin(abs(jetAxis) * 14.0 - uTime * 3.0));
    // Preserve the former far-field brightness at ds=.105; integrate by
    // distance so smaller steps near the horizon do not create extra energy.
    color += transmission * vec3(0.08, 0.42, 1.35) * jet * knot *
             (0.008 + 0.014 * uTreble) * (stepSize / 0.105);
    previousPlane = nextPlane;
    pos = nextPos;
    if (dot(pos, pos) > 121.0) break;
  }
  if (!swallowed) color += stars(dir) * transmission;
  float photonOffset = (closest - 0.79) * 24.0, outerOffset = (closest - 1.03) * 13.0;
  float photonRing = exp(-photonOffset * photonOffset);
  float outerRing = exp(-outerOffset * outerOffset);
  float caustic = 0.82 + 0.18 * sin(atan(p.y, p.x) * 9.0 - uTime * 0.7);
  color += vec3(1.55, 0.48, 0.09) * photonRing * caustic * (0.62 + 0.55 * uPulse);
  color += vec3(0.22, 0.08, 0.72) * outerRing * (0.22 + 0.30 * uMid);
  float lensArc = outerRing * pow(0.5 + 0.5 * cos(atan(p.y, p.x) * 2.0 + 0.4), 8.0);
  color += vec3(0.18, 0.48, 1.15) * lensArc * (0.10 + 0.18 * uTreble);
  float vignette = 1.0 - 0.36 * smoothstep(0.45, 1.45, length(p));
  outColor = vec4(max(color, vec3(0.0)) * vignette, 1.0);
}
)COSMIC";

inline constexpr const char* hyperVertex = R"COSMIC(#version 410 core
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec2 uRes;
uniform float uTime, uPulse, uBass, uMid, uTreble;
out vec3 vColor;
out float vOpacity;
out float vKind;
out float vDepth;

const float PI = 3.141592653589793;
const int FIELD = 65536;
const int SCAFFOLD = 16384;
uint hash(uint x) { x ^= x >> 16u; x *= 0x7feb352du; x ^= x >> 15u; x *= 0x846ca68bu; return x ^ (x >> 16u); }
float random(uint x) { return float(hash(x)) * (1.0 / 4294967295.0); }
vec3 hsv(float h, float s, float v) {
  vec3 p = abs(fract(vec3(h) + vec3(0.0, 2.0/3.0, 1.0/3.0)) * 6.0 - 3.0);
  return v * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), s);
}
vec4 rotate4(vec4 q, float a, float b) {
  float ca = cos(a), sa = sin(a), cb = cos(b), sb = sin(b);
  q.xz = mat2(ca, -sa, sa, ca) * q.xz;
  q.yw = mat2(cb, -sb, sb, cb) * q.yw;
  q.xw = mat2(cb, -sb, sb, cb) * q.xw;
  return q;
}
vec3 project4(vec4 q) {
  return q.xyz * (1.22 / max(1.15, 2.7 - 0.32 * q.w));
}
void main() {
  int id = gl_VertexID;
  vec3 p;
  float size = 2.0;
  vOpacity = 1.0;
  if (id < FIELD) {
    uint sid = uint(id);
    float echo = float(gl_InstanceID);
    vec4 q = vec4(random(sid * 4u), random(sid * 4u + 1u), random(sid * 4u + 2u), random(sid * 4u + 3u)) * PI;
    vec4 c = vec4(2.25 + 0.12 * uBass);
    float branchCount = 0.0;
    for (int iteration = 0; iteration < 28; ++iteration) {
      float choice = random(sid + uint(iteration) * 0x9e3779b9u);
      if (choice > 0.47 + 0.08 * uTreble) {
        q = 2.7182818 * sin(q) + c;
      } else {
        q = vec4(q.x * cos(q.y), q.y * sin(q.z), q.z * cos(q.w), q.w * sin(q.x));
        q = clamp(q, vec4(-10.0), vec4(10.0));
        branchCount += 1.0;
      }
    }
    q -= c;
    float delay = echo * (0.07 + 0.025 * uBass);
    q = rotate4(q, uTime * (0.09 + 0.018 * uMid) - delay, uTime * 0.061 + delay);
    p = project4(q) * (0.86 + 0.035 * uBass);
    float probability = branchCount / 28.0;
    vColor = hsv(fract(0.47 + q.w * 0.08 + probability * 0.58 + echo * 0.025), 0.82,
                 (0.68 + 0.40 * uTreble + 0.36 * uPulse) * exp(-echo * 0.42));
    vOpacity = exp(-echo * 0.55);
    vKind = 5.0;
    size = 1.55 + 1.35 * uPulse + 0.16 * echo;
  } else if (id < FIELD + SCAFFOLD) {
    int local = id - FIELD;
    int edge = local / 512;
    int sampleIndex = local % 512;
    int dimension = edge % 4;
    int pattern = edge / 4;
    vec4 q = vec4(0.0);
    int bitIndex = 0;
    for (int component = 0; component < 4; ++component) {
      if (component == dimension) q[component] = mix(-1.0, 1.0, float(sampleIndex) / 511.0);
      else {
        q[component] = ((pattern >> bitIndex) & 1) == 0 ? -1.0 : 1.0;
        ++bitIndex;
      }
    }
    q = rotate4(q * 1.55, uTime * 0.09, uTime * 0.061);
    p = project4(q);
    float packet = pow(max(0.0, sin(float(sampleIndex) * 0.061 - uTime * 2.2)), 10.0);
    vColor = mix(vec3(0.055, 0.30, 1.35), vec3(1.45, 0.08, 0.62), float(dimension) / 3.0) *
             (0.30 + 0.48 * packet + 0.32 * uMid + 0.24 * uPulse);
    vOpacity = 0.68; vKind = 1.0; size = 1.35 + 1.8 * packet + 0.7 * uPulse;
  } else {
    uint sid = uint(id - FIELD - SCAFFOLD);
    float a = random(sid * 3u) * 2.0 * PI, z = random(sid * 3u + 1u) * 2.0 - 1.0;
    float r = 5.5 + 3.0 * random(sid * 3u + 2u);
    p = vec3(sqrt(max(0.0, 1.0-z*z))*cos(a), z, sqrt(max(0.0, 1.0-z*z))*sin(a)) * r;
    vColor = mix(vec3(0.14, 0.46, 1.30), vec3(1.30, 0.62, 0.22), random(sid * 7u)) * (0.28 + uTreble);
    vOpacity = 0.7; vKind = 2.0; size = 1.4 + 1.6 * uTreble;
  }
  vec3 view = transpose(uCam) * (p - uEye);
  float depth = -view.z;
  vDepth = depth;
  float aspect = uRes.x / max(uRes.y, 1.0);
  gl_Position = depth > 0.05 ? vec4(view.xy / (depth * vec2(0.52 * aspect, 0.52)),
                                    clamp(depth / 12.0, 0.0, 1.0) * 2.0 - 1.0, 1.0)
                            : vec4(2.0, 2.0, 2.0, 1.0);
  gl_PointSize = depth > 0.05 ? clamp(size * uRes.y / (500.0 * depth), 1.0, 10.0) : 1.0;
}
)COSMIC";
}  // namespace cosmic_gpu
