#pragma once

// Native OpenGL 4.1 presentation of the parametric point effect supplied in
// 3d_aviation_navigation_visualizer_by_purpletwilight21858029.zip. The source
// page calls it "Hyperrealistic Quantum Fractal Explorer"; keep the visual
// attribution separate from the filename rather than inventing aviation data.
namespace aviation_gpu {
inline constexpr int uSteps = 192;
inline constexpr int vSteps = 96;
inline constexpr int lobes = 4;
inline constexpr int fieldPoints = (uSteps + 1) * (vSteps + 1) * lobes;
inline constexpr int moonPoints = 12288;
inline constexpr int markerPoints = 1536;
inline constexpr int ringPoints = 6144;
inline constexpr int starPoints = 4096;
inline constexpr int dustPoints = 4096;
inline constexpr int pointCount = fieldPoints + moonPoints + markerPoints + ringPoints + starPoints + dustPoints;

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
out float vMaterial;
out float vRoughness;
out float vDepth;

const float PI = 3.141592653589793;
const int U_STEPS = 192;
const int V_STEPS = 96;
const int LOBES = 4;
const int FIELD_POINTS = (U_STEPS + 1) * (V_STEPS + 1) * LOBES;
const int MOON_POINTS = 12288;
const int MARKER_POINTS = 1536;
const int RING_POINTS = 6144;
const int STAR_POINTS = 4096;

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
vec3 safeNormalize(vec3 v, vec3 fallback) {
  float lengthSquared = dot(v, v);
  return lengthSquared > 1e-10 ? v * inversesqrt(lengthSquared) : fallback;
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
  vMaterial = 0.0;
  vRoughness = 0.35;
  float wonder = 0.5 + 0.5 * sin(uTime * 0.035 - 1.2);
  float longing = 0.5 + 0.5 * sin(uTime * 0.017 + 2.0);
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
    float waveDv = cos(angle * 0.5 + v + phase);
    float ySignal = sin(u) * sin(v) * wave;
    p = vec3(cos(u) * sin(v) + sin(angle),
             sin(u) * cos(v) * wave + cos(angle),
             ySignal);
    vec3 du = vec3(-sin(u) * sin(v), cos(u) * cos(v) * wave, cos(u) * sin(v) * wave);
    vec3 dv = vec3(cos(u) * cos(v),
                   sin(u) * (-sin(v) * wave + cos(v) * waveDv),
                   sin(u) * ( cos(v) * wave + sin(v) * waveDv));
    vec3 normal = safeNormalize(cross(du, dv), safeNormalize(p, vec3(0.0, 0.0, 1.0)));
    float shell = 0.965 + 0.028 * sin(uTime * 0.16 + longing * PI) + 0.05 * uBass + echo * 0.006;
    p *= shell;
    float ax = (uTime - echoDelay) * 0.05;
    float ay = (uTime - echoDelay) * (0.10 + 0.012 * uMid);
    p = rotateField(p, ax, ay);
    normal = safeNormalize(rotateField(normal, ax, ay), vec3(0.0, 0.0, 1.0));
    vec3 eyeDirection = safeNormalize(uEye - p, vec3(0.0, 0.0, 1.0));
    vec3 key = normalize(vec3(-0.55, 0.72, 0.42));
    float diffuse = 0.24 + 0.76 * abs(dot(normal, key));
    float rim = pow(1.0 - abs(dot(normal, eyeDirection)), 2.4);
    float filament = pow(abs(sin(u * 4.0 + v * 7.0 + phase)), 10.0);
    float hue = fract(0.54 + 0.17 * wonder + 0.22 * ySignal + uTime * 0.006 - echo * 0.018);
    vec3 spectral = hsv(hue, 0.88 - 0.18 * rim, 1.0);
    vec3 warm = vec3(1.55, 0.28, 0.09);
    vColor = mix(spectral, warm, uPulse * (0.08 + 0.18 * filament)) *
             (0.55 + 0.58 * diffuse + 0.62 * rim + 0.34 * filament + 0.18 * uTreble) * exp(-echo * 0.48);
    vOpacity = exp(-echo * 0.62) * (0.78 + 0.22 * filament);
    vMaterial = mod(float(ui + vi + lobe), 11.0) < 2.0 ? 0.35 : 0.0;
    vRoughness = mix(0.08, 0.28, filament);
    size = 2.15 + 1.55 * uPulse + 0.55 * rim + echo * 0.22;
  } else if (id < FIELD_POINTS + MOON_POINTS) {
    vec3 n = fibonacciSphere(id - FIELD_POINTS, MOON_POINTS);
    p = n * 0.48 + vec3(2.35, 0.35, -1.45);
    float light = 0.12 + 0.88 * max(0.0, dot(n, normalize(vec3(-0.6, 0.8, 0.45))));
    float rim = pow(1.0 - abs(dot(n, normalize(uEye - p))), 2.3);
    float crater = smoothstep(0.62, 0.94, random(uint(id) * 13u) * 0.6 + 0.4 * sin(n.x * 31.0 + n.y * 19.0));
    vec3 stone = mix(vec3(0.17, 0.24, 0.34), vec3(0.42, 0.52, 0.68), light);
    vColor = stone * (1.0 - 0.38 * crater) + vec3(0.12, 0.28, 0.92) * rim * (0.65 + 0.2 * wonder);
    vMaterial = 1.0;
    vRoughness = 0.82;
    size = 3.15;
  } else if (id < FIELD_POINTS + MOON_POINTS + MARKER_POINTS) {
    vec3 n = fibonacciSphere(id - FIELD_POINTS - MOON_POINTS, MARKER_POINTS);
    float fall = mod(uTime * (0.8 + 0.18 * uBass), 4.0);
    float bounce = 1.6 - abs(fall - 2.0);
    p = n * 0.14 + vec3(-1.65, -0.95 + bounce, 0.25);
    vColor = mix(vec3(1.4, 0.04, 0.015), vec3(2.4, 0.7, 0.08),
                 max(0.0, dot(n, normalize(vec3(-0.4, 0.8, 0.5))))) * (1.0 + 0.5 * uPulse);
    vMaterial = 2.0;
    vRoughness = 0.16;
    size = 4.2;
  } else if (id < FIELD_POINTS + MOON_POINTS + MARKER_POINTS + RING_POINTS) {
    int rid = id - FIELD_POINTS - MOON_POINTS - MARKER_POINTS;
    int ring = rid / 1024;
    int sampleIndex = rid - ring * 1024;
    float a = float(sampleIndex) * (2.0 * PI / 1024.0);
    float radius = 1.36 + float(ring) * 0.23 + 0.035 * uPulse;
    p = vec3(cos(a) * radius, sin(a) * radius, 0.0);
    float tilt = 0.35 + float(ring) * 0.43;
    p = rotateField(p, tilt + 0.08 * sin(uTime * 0.13 + float(ring)),
                    uTime * (0.025 + 0.006 * float(ring)) + float(ring) * 0.7);
    float packet = pow(max(0.0, cos(a * (4.0 + float(ring)) - uTime * (1.2 + 0.2 * uMid))), 18.0);
    vColor = mix(vec3(0.035, 0.12, 0.31), hsv(0.50 + 0.041 * float(ring) + 0.05 * wonder, 0.72, 1.35),
                 0.32 + 0.68 * packet) * (0.55 + 0.25 * uMid + 0.30 * uPulse);
    vOpacity = 0.42 + 0.48 * packet;
    vMaterial = 3.0;
    vRoughness = 0.06;
    size = 1.35 + 1.85 * packet + 0.6 * uPulse;
  } else if (id < FIELD_POINTS + MOON_POINTS + MARKER_POINTS + RING_POINTS + STAR_POINTS) {
    uint sid = uint(id - FIELD_POINTS - MOON_POINTS - MARKER_POINTS - RING_POINTS);
    float a = random(sid * 3u) * 2.0 * PI;
    float z = random(sid * 3u + 1u) * 2.0 - 1.0;
    float r = 5.5 + random(sid * 3u + 2u) * 3.0;
    p = vec3(sqrt(max(0.0, 1.0 - z * z)) * cos(a), z,
             sqrt(max(0.0, 1.0 - z * z)) * sin(a)) * r;
    float temperature = random(sid * 7u + 4u);
    vColor = mix(vec3(0.20, 0.44, 1.35), vec3(1.35, 0.74, 0.34), temperature) *
             (0.38 + 0.95 * uTreble + 0.25 * uPulse);
    vOpacity = 0.52 + 0.35 * random(sid * 5u);
    vMaterial = 4.0;
    vRoughness = 0.02;
    size = 1.55 + 1.65 * uTreble + 0.8 * step(0.96, temperature);
  } else {
    uint did = uint(id - FIELD_POINTS - MOON_POINTS - MARKER_POINTS - RING_POINTS - STAR_POINTS);
    float a = random(did * 3u) * 2.0 * PI;
    float band = random(did * 3u + 1u) * 2.0 - 1.0;
    float r = 2.0 + 4.8 * pow(random(did * 3u + 2u), 0.72);
    float spiral = a + r * 0.42 + uTime * 0.012;
    p = vec3(cos(spiral) * r, band * (0.35 + r * 0.16), sin(spiral) * r);
    p = rotateField(p, 0.28, -0.24);
    vColor = mix(vec3(0.055, 0.12, 0.34), vec3(0.42, 0.08, 0.52), longing) *
             (0.20 + 0.42 * uBass);
    vOpacity = 0.16 + 0.20 * random(did * 11u);
    vMaterial = 5.0;
    vRoughness = 1.0;
    size = 1.0 + 1.2 * uBass;
  }

  vec3 view = transpose(uCam) * (p - uEye);
  float depth = -view.z;
  vDepth = depth;
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
in float vMaterial;
in float vRoughness;
in float vDepth;
out vec4 outColor;
void main() {
  vec2 p = gl_PointCoord * 2.0 - 1.0;
  float radius = dot(p, p);
  if (radius > 1.0) discard;
  float sphereZ = sqrt(max(0.0, 1.0 - radius));
  vec3 normal = vec3(p, sphereZ);
  vec3 lightDir = normalize(vec3(-0.48, 0.64, 0.72));
  float diffuse = 0.22 + 0.78 * max(0.0, dot(normal, lightDir));
  float specular = pow(max(0.0, dot(reflect(-lightDir, normal), vec3(0.0, 0.0, 1.0))),
                       mix(52.0, 5.0, vRoughness));
  float rim = pow(1.0 - sphereZ, 2.0);
  float halo = 1.0 - smoothstep(0.10, 1.0, radius);
  float core = 1.0 - smoothstep(0.0, 0.18, radius);
  vec3 shaded = vColor;
  float energy = 0.0;
  if (vMaterial < 0.2) {
    float caustic = pow(max(0.0, sin((p.x - p.y) * 8.0 + radius * 11.0)), 8.0);
    shaded *= 0.38 + 0.60 * diffuse + 0.85 * specular + 0.48 * rim + 0.24 * caustic;
    energy = 0.24 * halo + 0.88 * core + 0.22 * rim;
  } else if (vMaterial < 0.5) {
    shaded *= 0.18 + 0.32 * diffuse + 0.24 * specular + 0.92 * rim;
    energy = 0.16 * halo + 0.42 * core + 0.48 * rim;
  } else if (vMaterial < 1.5) {
    shaded *= 0.42 + 0.68 * diffuse + 0.16 * specular;
    energy = 0.64 * halo + 0.18 * core;
  } else if (vMaterial < 2.5) {
    shaded *= 0.55 + 0.44 * diffuse + 1.25 * specular;
    energy = 0.38 * halo + 1.18 * core + 0.32 * rim;
  } else if (vMaterial < 3.5) {
    float anisotropic = pow(max(0.0, 1.0 - abs(p.y + p.x * 0.28)), 22.0);
    shaded *= 0.28 + 0.38 * diffuse + 1.20 * specular + 0.58 * anisotropic;
    energy = 0.34 * halo + 0.82 * core;
  } else if (vMaterial < 4.5) {
    float flare = pow(max(0.0, 1.0 - abs(p.x)), 14.0) + pow(max(0.0, 1.0 - abs(p.y)), 14.0);
    energy = 0.18 * halo + 1.28 * core + 0.18 * flare;
  } else {
    energy = 0.17 * halo;
  }
  float fog = smoothstep(5.0, 10.5, vDepth);
  shaded = mix(shaded, vec3(0.035, 0.07, 0.18), fog * 0.46);
  float coverage = halo * vOpacity;
  outColor = vec4(shaded * energy * vOpacity, coverage);
}
)AVIATION";

// GPU-only feedback for luminous motion persistence. A tiny rotating expansion
// produces curved afterimages while exponential decay keeps energy bounded.
inline constexpr const char* temporal = R"AVIATION(#version 410 core
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uCurrent, uHistory;
uniform float uWeight, uPulse, uTime;
uniform vec2 uTexel;
void main() {
  vec3 current = texture(uCurrent, vUv).rgb;
  if (uWeight <= 0.0) {
    outColor = vec4(current, 1.0);
    return;
  }
  vec2 p = vUv - 0.5;
  float angle = 0.0018 + 0.0012 * uPulse + 0.0005 * sin(uTime * 0.3);
  float c = cos(angle), s = sin(angle);
  vec2 historyUv = 0.5 + mat2(c, -s, s, c) * p * (1.0020 + 0.0014 * uPulse);
  vec2 chroma = vec2(0.52 + 0.26 * uPulse, 0.0) * uTexel;
  vec3 history = texture(uHistory, historyUv).rgb;
  history.r = texture(uHistory, historyUv + chroma).r;
  history.b = texture(uHistory, historyUv - chroma).b;
  vec2 radial = p + vec2(1e-4);
  radial *= inversesqrt(max(dot(radial, radial), 1e-8));
  vec3 curvedEcho = texture(uHistory, historyUv - radial * uTexel * 1.35).rgb;
  history = max(history, curvedEcho * 0.82);
  vec3 persistent = min(history, vec3(5.0)) * uWeight;
  vec3 resonance = min(current, persistent) * (0.055 + 0.035 * uPulse);
  outColor = vec4(max(current, persistent) + resonance, 1.0);
}
)AVIATION";
}  // namespace aviation_gpu
