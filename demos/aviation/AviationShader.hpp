#pragma once

// Native OpenGL 4.1 presentation of the parametric point effect supplied in
// 3d_aviation_navigation_visualizer_by_purpletwilight21858029.zip. The source
// page calls it "Hyperrealistic Quantum Fractal Explorer"; keep the visual
// attribution separate from the filename rather than inventing aviation data.
namespace aviation_gpu {
inline constexpr int uSteps = 160;
inline constexpr int vSteps = 80;
inline constexpr int lobes = 4;
inline constexpr int fieldPoints = (uSteps + 1) * (vSteps + 1) * lobes;
inline constexpr int moonPoints = 8192;
inline constexpr int markerPoints = 1024;
inline constexpr int ringPoints = 4096;
inline constexpr int starPoints = 3072;
inline constexpr int pointCount = fieldPoints + moonPoints + markerPoints + ringPoints + starPoints;

inline constexpr const char* vertex = R"AVIATION(#version 410 core
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec2 uRes;
uniform float uTime;
uniform float uPulse;
uniform float uBass;
uniform float uMid;
uniform float uTreble;
out vec3 vColor;
out float vOpacity;

const float PI = 3.141592653589793;
const int U_STEPS = 160;
const int V_STEPS = 80;
const int LOBES = 4;
const int FIELD_POINTS = (U_STEPS + 1) * (V_STEPS + 1) * LOBES;
const int MOON_POINTS = 8192;
const int MARKER_POINTS = 1024;
const int RING_POINTS = 4096;

uint hash(uint x) {
  x ^= x >> 16u;
  x *= 0x7feb352du;
  x ^= x >> 15u;
  x *= 0x846ca68bu;
  return x ^ (x >> 16u);
}
float random(uint x) { return float(hash(x)) * (1.0 / 4294967295.0); }
vec3 hsv(float h, float s, float v) {
  vec3 p = abs(fract(vec3(h) + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
  return v * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), s);
}
vec3 rotateField(vec3 p, float ax, float ay) {
  float cx = cos(ax), sx = sin(ax), cy = cos(ay), sy = sin(ay);
  p.yz = mat2(cx, -sx, sx, cx) * p.yz;
  p.xz = mat2(cy, -sy, sy, cy) * p.xz;
  return p;
}
vec3 fibonacciSphere(int index, int count) {
  float q = (float(index) + 0.5) / float(count);
  float z = 1.0 - 2.0 * q;
  float a = float(index) * 2.399963229728653;
  float r = sqrt(max(0.0, 1.0 - z * z));
  return vec3(r * cos(a), z, r * sin(a));
}

void main() {
  int id = gl_VertexID;
  vec3 p;
  float size = 2.0;
  vOpacity = 1.0;
  if (id < FIELD_POINTS) {
    float echo = float(gl_InstanceID);
    int lobe = id % LOBES;
    int cell = id / LOBES;
    int ui = cell % (U_STEPS + 1);
    int vi = cell / (U_STEPS + 1);
    float u = float(ui) * (2.0 * PI / float(U_STEPS));
    float v = float(vi) * (PI / float(V_STEPS));
    float angle = float(lobe) * PI * 0.5;
    float echoDelay = echo * (0.055 + 0.025 * uBass);
    float phase = (uTime - echoDelay) * 0.5 + 0.16 * uPulse + 0.08 * uBass;
    float wave = sin(angle * 0.5 + v + phase);
    float ySignal = sin(u) * sin(v) * wave;
    p = vec3(cos(u) * sin(v) + sin(angle),
             sin(u) * cos(v) * wave + cos(angle),
             ySignal);
    p *= 0.94 + 0.055 * uBass + echo * 0.006;
    p = rotateField(p, (uTime - echoDelay) * 0.05,
                    (uTime - echoDelay) * (0.10 + 0.012 * uMid));
    vColor = hsv(fract(0.5 * (ySignal + 1.0) + uTime * 0.012 - echo * 0.018), 0.94,
                 (1.15 + 0.45 * uPulse + 0.18 * uTreble) * exp(-echo * 0.48));
    vOpacity = exp(-echo * 0.62);
    size = 2.3 + 1.7 * uPulse + echo * 0.22;
  } else if (id < FIELD_POINTS + MOON_POINTS) {
    vec3 n = fibonacciSphere(id - FIELD_POINTS, MOON_POINTS);
    p = n * 0.48 + vec3(2.35, 0.35, -1.45);
    float light = 0.18 + 0.82 * max(0.0, dot(n, normalize(vec3(-0.6, 0.8, 0.45))));
    float rim = pow(1.0 - abs(dot(n, normalize(uEye - p))), 2.0);
    vColor = vec3(0.28, 0.42, 0.62) * light + vec3(0.15, 0.3, 0.7) * rim;
    size = 3.0;
  } else if (id < FIELD_POINTS + MOON_POINTS + MARKER_POINTS) {
    vec3 n = fibonacciSphere(id - FIELD_POINTS - MOON_POINTS, MARKER_POINTS);
    float fall = mod(uTime * (0.8 + 0.18 * uBass), 4.0);
    float bounce = 1.6 - abs(fall - 2.0);
    p = n * 0.14 + vec3(-1.65, -0.95 + bounce, 0.25);
    vColor = mix(vec3(1.4, 0.04, 0.015), vec3(2.4, 0.7, 0.08),
                 max(0.0, dot(n, normalize(vec3(-0.4, 0.8, 0.5))))) * (1.0 + 0.5 * uPulse);
    size = 4.2;
  } else if (id < FIELD_POINTS + MOON_POINTS + MARKER_POINTS + RING_POINTS) {
    int rid = id - FIELD_POINTS - MOON_POINTS - MARKER_POINTS;
    int ring = rid / 1024;
    int sampleIndex = rid - ring * 1024;
    float a = float(sampleIndex) * (2.0 * PI / 1024.0);
    float radius = 1.42 + float(ring) * 0.29 + 0.035 * uPulse;
    p = vec3(cos(a) * radius, sin(a) * radius, 0.0);
    float tilt = 0.35 + float(ring) * 0.43;
    p = rotateField(p, tilt + 0.08 * sin(uTime * 0.13 + float(ring)),
                    uTime * (0.025 + 0.006 * float(ring)) + float(ring) * 0.7);
    float packet = pow(max(0.0, cos(a * (4.0 + float(ring)) - uTime * (1.2 + 0.2 * uMid))), 18.0);
    vColor = mix(vec3(0.04, 0.18, 0.42), hsv(0.52 + 0.055 * float(ring), 0.8, 1.25),
                 0.32 + 0.68 * packet) * (0.55 + 0.25 * uMid + 0.30 * uPulse);
    vOpacity = 0.42 + 0.48 * packet;
    size = 1.5 + 1.8 * packet + 0.6 * uPulse;
  } else {
    uint sid = uint(id - FIELD_POINTS - MOON_POINTS - MARKER_POINTS - RING_POINTS);
    float a = random(sid * 3u) * 2.0 * PI;
    float z = random(sid * 3u + 1u) * 2.0 - 1.0;
    float r = 5.5 + random(sid * 3u + 2u) * 3.0;
    p = vec3(sqrt(max(0.0, 1.0 - z * z)) * cos(a), z,
             sqrt(max(0.0, 1.0 - z * z)) * sin(a)) * r;
    vColor = vec3(0.18, 0.42, 1.10) * (0.45 + 0.90 * uTreble + 0.18 * uPulse);
    vOpacity = 0.65;
    size = 1.8 + 1.5 * uTreble;
  }

  vec3 view = transpose(uCam) * (p - uEye);
  float depth = -view.z;
  float aspect = uRes.x / max(uRes.y, 1.0);
  if (depth <= 0.05) {
    gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    gl_PointSize = 1.0;
    vOpacity = 0.0;
  } else {
    vec2 ndc = view.xy / (depth * vec2(0.52 * aspect, 0.52));
    gl_Position = vec4(ndc, clamp(depth / 10.0, 0.0, 1.0) * 2.0 - 1.0, 1.0);
    gl_PointSize = clamp(size * uRes.y / (500.0 * depth), 1.0, 9.0);
  }
}
)AVIATION";

inline constexpr const char* fragment = R"AVIATION(#version 410 core
in vec3 vColor;
in float vOpacity;
out vec4 outColor;
void main() {
  vec2 p = gl_PointCoord * 2.0 - 1.0;
  float radius = dot(p, p);
  float halo = 1.0 - smoothstep(0.08, 1.0, radius);
  float core = 1.0 - smoothstep(0.0, 0.16, radius);
  float coverage = halo * vOpacity;
  outColor = vec4(vColor * (0.32 * halo + 0.92 * core) * vOpacity, coverage);
}
)AVIATION";

// GPU-only feedback for luminous motion persistence. A tiny rotating expansion
// produces curved afterimages while exponential decay keeps energy bounded.
inline constexpr const char* temporal = R"AVIATION(#version 410 core
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uCurrent, uHistory;
uniform float uWeight, uPulse, uTime;
void main() {
  vec3 current = texture(uCurrent, vUv).rgb;
  if (uWeight <= 0.0) {
    outColor = vec4(current, 1.0);
    return;
  }
  vec2 p = vUv - 0.5;
  float angle = 0.0018 + 0.0012 * uPulse + 0.0005 * sin(uTime * 0.3);
  float c = cos(angle), s = sin(angle);
  vec2 historyUv = 0.5 + mat2(c, -s, s, c) * p * (1.0022 + 0.0012 * uPulse);
  vec3 history = texture(uHistory, historyUv).rgb;
  history.r = texture(uHistory, historyUv + vec2(0.00045, 0.0)).r;
  history.b = texture(uHistory, historyUv - vec2(0.00045, 0.0)).b;
  vec3 persistent = history * uWeight;
  outColor = vec4(max(current, persistent) + min(current, persistent) * 0.06, 1.0);
}
)AVIATION";
}  // namespace aviation_gpu
