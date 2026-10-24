#pragma once

// Native GPU interpretations of the user-supplied Möbius universe and advanced
// quantum-fractal browser studies. Geometry is generated from gl_VertexID so
// animation never rebuilds or uploads vertex buffers.
namespace cosmic_gpu {
inline constexpr int mobiusU = 256;
inline constexpr int mobiusV = 64;
inline constexpr int mobiusSurfaceVertices = mobiusU * mobiusV * 6;
inline constexpr int mobiusCorePoints = 8192;
inline constexpr int mobiusHorizonPoints = 4096;
inline constexpr int mobiusStarPoints = 8192;
inline constexpr int mobiusDebrisPoints = 4096;
inline constexpr int mobiusFlowPoints = 16384;
inline constexpr int mobiusPointCount = mobiusCorePoints + mobiusHorizonPoints + mobiusStarPoints +
                                        mobiusDebrisPoints + mobiusFlowPoints;

inline constexpr int quantumFieldPoints = 98304;
inline constexpr int quantumBraidPoints = 8192;
inline constexpr int quantumNodePoints = 4096;
inline constexpr int quantumStarPoints = 4096;
inline constexpr int quantumPointCount = quantumFieldPoints + quantumBraidPoints + quantumNodePoints +
                                         quantumStarPoints;

inline constexpr const char* mobiusSurfaceVertex = R"COSMIC(#version 410 core
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec2 uRes;
uniform float uTime, uPulse, uBass, uMid, uTreble;
out vec3 vNormal;
out vec3 vView;
out vec3 vBase;
out float vFlow;
out float vDepth;

const float PI = 3.141592653589793;
const int U_CELLS = 256;
const int V_CELLS = 64;

vec3 mobius(float u, float v, float time, float bass) {
  float twist = sin(u * 0.5) * 2.5;
  float ribbonAngle = u * 0.5 + twist;
  float detail = 0.018 * sin(u * 20.0 + time * 0.7) + 0.016 * cos(v * 15.0 - time * 0.31);
  float radius = 1.5 + 0.5 * v * cos(ribbonAngle);
  vec3 p = vec3(radius * cos(u), radius * sin(u), 0.5 * v * sin(ribbonAngle));
  p.z += 0.25 * cos(u * 0.25);
  p *= 0.82 * (1.0 + detail + 0.018 * bass);
  return p;
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
  vec3 p = mobius(u, v, time, uBass);
  float eu = 4.0 * PI / float(U_CELLS);
  float ev = 2.0 / float(V_CELLS);
  vec3 tangentU = mobius(u + eu, v, time, uBass) - mobius(u - eu, v, time, uBass);
  vec3 tangentV = mobius(u, v + ev, time, uBass) - mobius(u, v - ev, time, uBass);
  vNormal = safeNormalize(cross(tangentU, tangentV), vec3(0.0, 0.0, 1.0));
  vView = safeNormalize(uEye - p, vec3(0.0, 0.0, 1.0));
  float flow = 0.5 + 0.5 * sin(u * 7.0 - uTime * (2.4 + 0.8 * uMid));
  float dimension = 0.5 + 0.5 * cos(u * 0.25 + v * PI);
  vBase = mix(vec3(0.025, 0.20, 0.58), vec3(0.76, 0.035, 0.34), dimension);
  vBase = mix(vBase, vec3(0.12, 0.86, 1.20), flow * (0.22 + 0.30 * uTreble));
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
in float vFlow;
in float vDepth;
out vec4 outColor;
uniform float uPulse, uBass, uTreble;
void main() {
  vec3 n = normalize(vNormal);
  vec3 v = normalize(vView);
  vec3 l = normalize(vec3(-0.42, 0.74, 0.52));
  vec3 h = normalize(l + v);
  float ndl = max(0.0, dot(n, l));
  float ndh = max(0.0, dot(n, h));
  float nv = abs(dot(n, v));
  float fresnel = pow(1.0 - nv, 3.0);
  float metal = pow(ndh, 48.0) * (0.45 + 0.55 * ndl);
  float weave = 0.5 + 0.5 * sin((gl_FragCoord.x + gl_FragCoord.y) * 0.18);
  vec3 body = vBase * (0.12 + 0.34 * ndl + 0.18 * weave);
  vec3 edge = mix(vec3(0.04, 0.62, 1.35), vec3(1.35, 0.12, 0.48), vFlow);
  vec3 emission = edge * (0.24 + 0.82 * fresnel + 0.42 * metal + 0.24 * uPulse + 0.12 * uTreble);
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
const int STARS = 8192;
const int DEBRIS = 4096;
uint hash(uint x) { x ^= x >> 16u; x *= 0x7feb352du; x ^= x >> 15u; x *= 0x846ca68bu; return x ^ (x >> 16u); }
float random(uint x) { return float(hash(x)) * (1.0 / 4294967295.0); }
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
    float rim = pow(1.0 - abs(dot(n, normalize(uEye - p))), 2.0);
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
  float halo = 1.0 - smoothstep(0.08, 1.0, r2);
  float core = 1.0 - smoothstep(0.0, 0.14, r2);
  float flare = pow(max(0.0, 1.0 - abs(p.x)), 18.0) + pow(max(0.0, 1.0 - abs(p.y)), 18.0);
  float spec = pow(max(0.0, dot(normalize(vec3(-0.4, 0.7, 0.65)), vec3(p, z))), 18.0);
  float energy = vKind < 0.5 ? 0.28 * halo + 0.72 * spec + core :
                 vKind < 1.5 ? 0.34 * halo + 0.42 * core + 0.30 * (1.0 - z) :
                 vKind < 2.5 ? 0.18 * halo + 0.86 * core + 0.12 * flare :
                 vKind < 3.5 ? 0.42 * halo + 0.30 * spec :
                 vKind < 4.5 ? 0.25 * halo + 0.74 * core : 0.10 * halo + 0.34 * core;
  float fog = smoothstep(6.0, 12.0, vDepth);
  vec3 color = mix(vColor, vec3(0.012, 0.025, 0.085), fog * 0.48);
  outColor = vec4(color * energy * vOpacity, halo * vOpacity);
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
}  // namespace cosmic_gpu
