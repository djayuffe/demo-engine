#pragma once
#include <string>
#include <stdexcept>
namespace dmt::gpu {
inline constexpr int modes = 10;
inline constexpr const char* fragment = R"GLSL(#version 410 core
in vec2 vUv;
out vec4 outColor;
uniform vec2 uRes;
uniform vec3 uEye;
uniform mat3 uCam;
uniform mat4 uRotation;
uniform float uTime, uPulse, uBass, uExposure;
uniform int uMode, uUseBvh, uEmpty;
uniform vec4 uPlanes[8], uBounds[112], uMeta[56], uLevels[9], uEvents[24];
const float PI = 3.14159265359;
vec3 palette(float t) { return .5 + .5 * cos(6.2831853 * (t + vec3(0, .33, .67))); }
mat2 rot(float a) { return mat2(cos(a), -sin(a), sin(a), cos(a)); }
float line(float d, float width, float aa) { return 1 - smoothstep(width, width + max(aa, .0001), abs(d)); }
float segment(vec3 ro, vec3 rd, vec3 a, vec3 b, out float depth) {
  vec3 v = b - a, w = ro - a;
  float vv = dot(v, v), dv = dot(rd, v), den = max(vv - dv * dv, 1e-8);
  float t = clamp((dot(w, v) - dv * dot(rd, w)) / den, 0, 1);
  vec3 q = a + t * v;
  depth = max(0, dot(q - ro, rd));
  return length(ro + rd * depth - q);
}
vec3 section(vec3 ro, vec3 rd) {
  if (uEmpty != 0)
    return vec3(0);
  float enter = 0, exit = 100;
  int face = 0;
  for (int i = 0; i < 8; i++) {
    vec4 p = uPlanes[i];
    float a = p.w - dot(p.xyz, ro), b = dot(p.xyz, rd);
    if (abs(b) < 1e-7) {
      if (a < 0)
        return vec3(0);
      continue;
    }
    float t = a / b;
    if (b < 0) {
      if (t > enter) {
        enter = t;
        face = i;
      }
    } else
      exit = min(exit, t);
  }
  if (exit < enter || enter <= 0)
    return vec3(0);
  vec3 hit = ro + rd * enter, n = normalize(uPlanes[face].xyz);
  float edge = 100;
  for (int i = 0; i < 8; i++) {
    float len = length(uPlanes[i].xyz);
    if (i != face && len > 1e-6)
      edge = min(edge, abs(uPlanes[i].w - dot(uPlanes[i].xyz, hit)) / len);
  }
  float aa = enter / uRes.y * 1.5;
  float wire = line(edge, .008, aa);
  float grid = line(sin(dot(hit, vec3(9, 7, 11)) + uTime * .2), .025, aa * 12);
  float light = .12 + .65 * max(0, dot(n, normalize(vec3(-.6, .8, 1))));
  return palette(float(face) * .13 + uTime * .015) *
         (light * .24 + wire * (1.5 + .15 * uPulse) + exp(-edge * 32) * .2 + grid * .05);
}
vec3 hypercube(vec3 ro, vec3 rd) {
  vec3 color = vec3(0);
  for (int i = 0; i < 16; i++)
    for (int axis = 0; axis < 4; axis++) {
      int j = i ^ (1 << axis);
      if (i >= j)
        continue;
      vec4 a, b;
      for (int k = 0; k < 4; k++) {
        a[k] = (i & (1 << k)) != 0 ? 1 : -1;
        b[k] = (j & (1 << k)) != 0 ? 1 : -1;
      }
      a = uRotation * a;
      b = uRotation * b;
      float limit = 3.95;
      bool ia = a.w <= limit, ib = b.w <= limit;
      if (!ia && !ib)
        continue;
      if (ia != ib) {
        vec4 p = mix(a, b, (limit - a.w) / (b.w - a.w));
        if (!ia)
          a = p;
        else
          b = p;
      }
      vec3 pa = a.xyz * 4 / (4 - a.w), pb = b.xyz * 4 / (4 - b.w);
      float t;
      float d = segment(ro, rd, pa, pb, t);
      if (t <= 0)
        continue;
      float aa = t / uRes.y;
      float core = line(d, .008, aa);
      color +=
          palette(float(axis) * .21 + uTime * .015) * (core * 1.3 + .12 * exp(-d * 35)) / (1 + .04 * t * t);
    }
  return color;
}
vec3 radial(vec2 p) {
  float r = max(length(p), .001), a = atan(p.y, p.x), sum = 0;
  for (int k = 0; k < 9; k++) {
    vec4 l = uLevels[k];
    float phase = l.x * a + uTime * (.25 + float(k) * .04) + l.y + l.z * log(r);
    float footprint = length(vec2(l.x, l.z)) / (r * uRes.y);
    float lod = 1 - smoothstep(.25, 1.5, footprint);
    sum += l.w * sin(phase) * lod;
  }
  float ridge = pow(.5 + .5 * cos(sum * 4), 4);
  float iris = exp(-r * r * .6);
  return palette(.62 + sum * .10 + uTime * .02) * (ridge * .85 + abs(tanh(sum * 1.5)) * .18) * iris *
         (.85 + .12 * uPulse);
}
vec3 folds(vec2 p) {
  float accum = 0;
  float footprint = 2 / uRes.y;
  for (int k = 0; k < 7; k++) {
    p = rot(uTime * .1 + float(k) * .73) * p;
    p = (abs(p) - vec2(.57, .49)) * 1.22;
    footprint *= 1.22;
    float d = min(abs(p.x), abs(p.y));
    float intensity = exp(-55 * d * d);
    float fine = 1 - smoothstep(.025, .15, footprint);
    accum += intensity * .15 * fine;
  }
  return vec3(.33, .85, .72) * pow(accum, 2) * 1.3;
}
float lor(vec4 a, vec4 b) { return -a.x * b.x + dot(a.yzw, b.yzw); }
vec4 lift(vec3 p) { return vec4(sqrt(1 + dot(p, p)), p); }
vec4 tangent(vec4 o, vec3 v) {
  float d = dot(o.yzw, v);
  return vec4(d, v + o.yzw * d / (o.x + 1));
}
float sphere(vec4 o, vec4 d, vec4 packed, bool bound) {
  vec4 c = lift(packed.xyz);
  float A = -lor(o, c), B = -lor(d, c), C = cosh(packed.w);
  if (bound && A <= C)
    return 0;
  float aa = A + B, cc = A - B, disc = C * C - aa * cc;
  if (disc < 0)
    return 1e10;
  float root = sqrt(max(0, disc)), best = 1e10;
  float u = cc / (C + root);
  if (u > 1.000001)
    best = log(u);
  if (abs(aa) > 1e-8) {
    u = (C + root) / aa;
    if (u > 1.000001)
      best = min(best, log(u));
  } else {
    u = cc / (2 * C);
    if (u > 1.000001)
      best = min(best, log(u));
  }
  return best;
}
vec4 primitive(int i) {
  float f = float(i), phi = f * 2.39996323;
  return vec4(.92 * cos(phi), .92 * sin(phi), 1.7 + float(i % 8) * .44, .16 + .035 * float(i % 4));
}
vec3 h3(vec2 p, float view) {
  float turn = uTime * .09;
  vec3 origin = uCam * (uEye * .22 + vec3(view * .38, 0, 0));
  vec3 direction = normalize(uCam * vec3(p * .92, 1.25));
  origin.xy = rot(turn) * origin.xy;
  direction.xy = rot(turn) * direction.xy;
  vec4 o = lift(origin), d = tangent(o, direction);
  float nearest = 1e10;
  int material = -1;
  if (uUseBvh != 0) {
    int index = 0;
    for (int iteration = 0; iteration < 112; iteration++) {
      if (index >= 111)
        break;
      vec4 meta = uMeta[index / 2];
      vec2 info = (index % 2) == 0 ? meta.xy : meta.zw;
      float entry = sphere(o, d, uBounds[index], true);
      if (entry >= 1e9 || entry > nearest) {
        index = int(info.x + .5);
        continue;
      }
      int leaf = int(info.y + .5) - 1;
      if (leaf >= 0) {
        float t = sphere(o, d, primitive(leaf), false);
        if (t < nearest) {
          nearest = t;
          material = leaf;
        }
      }
      index++;
    }
  } else
    for (int i = 0; i < 56; i++) {
      float t = sphere(o, d, primitive(i), false);
      if (t < nearest) {
        nearest = t;
        material = i;
      }
    }
  if (material < 0) {
    float r = length(p), a = atan(p.y, p.x);
    float ridge = pow(abs(sin(15 * a + 8 * log(r + .02) - uTime)), 18);
    float coverage = 1 - smoothstep(.2, 1, 15 / (max(r, .01) * uRes.y));
    return vec3(.02, .13, .25) * ridge * coverage;
  }
  vec4 s = primitive(material), center = lift(s.xyz);
  vec4 hit = o * cosh(nearest) + d * sinh(nearest);
  vec4 n = (cosh(s.w) * hit - center) / sinh(s.w);
  // Transport the normal and viewing tangent to the hit point's rest frame.
  vec3 normal = normalize(n.yzw - hit.yzw * n.x / (hit.x + 1));
  vec4 viewT = -(o * sinh(nearest) + d * cosh(nearest));
  vec3 v = normalize(viewT.yzw - hit.yzw * viewT.x / (hit.x + 1));
  vec3 light = normalize(vec3(-.5, .8, -.7));
  float diffuse = max(.06, dot(normal, light));
  vec3 halfv = light + v;
  float spec = pow(max(0, dot(normal, normalize(halfv + vec3(1e-6)))), 52);
  float bands = .5 + .5 * sin(22 * atan(normal.y, normal.x) + uTime + 14 * normal.z);
  vec3 color = mix(vec3(.08, .55, .95), vec3(.78, .13, .44), bands);
  return (color * (diffuse * .85 + .035) + vec3(.6, .8, 1) * spec * .65) * exp(-nearest * .1) *
         (1 + .1 * uPulse);
}
vec3 ornament(vec2 p) {
  float r = length(p), a = atan(p.y, p.x);
  float angle = 14 * a + uTime + 3 * log(r + .01), rad = 24 * r - uTime * .4;
  float footprint = (14 / max(r, .01) + 24) / uRes.y;
  float edge = pow(abs(sin(angle) * cos(rad)), 8) * (1 - smoothstep(.3, 1, footprint));
  return palette(.65 + r * .2 + uTime * .012) * edge * .8;
}
// Authored time is the depth axis. Node i inherits a causal edge from (i-1)/2.
// Glyph grammar: each node expands into a seeded 3..6-sided orbital mark.
vec3 events(vec3 ro, vec3 rd) {
  vec3 color = vec3(0);
  float clock = mod(uTime, 24);
  for (int i = 0; i < 24; i++) {
    vec4 e = uEvents[i];
    float age = clock - e.w;
    float life = smoothstep(0, .35, age) * (1 - smoothstep(2.8, 4, age));
    float depth;
    float d = segment(ro, rd, e.xyz, e.xyz + vec3(0, .001, 0), depth);
    vec3 tint = palette(float(i) * .11);
    if (depth > 0)
      color += tint * (exp(-d * d / .006) + .12 * exp(-d * 13)) * (life + .16);
    int sides = 3 + i % 4;
    for (int j = 0; j < 6; j++) {
      if (j >= sides)
        break;
      float a = float(j) * 2 * PI / float(sides), b = float(j + 1) * 2 * PI / float(sides);
      vec3 ga = e.xyz + vec3(cos(a), sin(a), 0) * .095;
      vec3 gb = e.xyz + vec3(cos(b), sin(b), 0) * .095;
      d = segment(ro, rd, ga, gb, depth);
      if (depth > 0)
        color += tint * line(d, .003, depth / uRes.y) * (.08 + .3 * life);
    }
    if (i > 0) {
      vec4 parent = uEvents[(i - 1) / 2];
      d = segment(ro, rd, parent.xyz, e.xyz, depth);
      if (depth > 0)
        color += tint * line(d, .004, depth / uRes.y) * .11;
      float progress = clamp(age * .8, 0, 1);
      vec3 end = mix(parent.xyz, e.xyz, progress);
      d = segment(ro, rd, parent.xyz, end, depth);
      if (depth > 0)
        color += tint * line(d, .007, depth / uRes.y) * life * .65;
    }
  }
  return color;
}
float ellipsoid(vec3 p, vec3 radius) {
  float k0 = length(p / radius), k1 = length(p / (radius * radius));
  return k0 * (k0 - 1) / max(k1, 1e-6);
}
float face(vec3 p) {
  float head = ellipsoid(p, vec3(.85, 1.2, .65));
  vec3 eyes = p - vec3(0, .25, .58);
  eyes.x = abs(eyes.x) - .32;
  float sockets = length(eyes / vec3(1, .65, .8)) - .22;
  head = max(head, -sockets);
  float nose = ellipsoid(p - vec3(0, .04, .66), vec3(.13, .32, .22));
  float mouth = length(vec3(max(abs(p.x) - .25, 0), p.y + .4, p.z - .59)) - .055;
  return max(min(head, nose), -mouth);
}
vec3 emergence(vec3 ro, vec3 rd) {
  float t = 0;
  vec3 hit;
  bool found = false;
  for (int i = 0; i < 96; i++) {
    hit = ro + rd * t;
    float d = face(hit);
    if (abs(d) < .0015) {
      found = true;
      break;
    }
    t += max(abs(d) * .65, .0005);
    if (t > 12)
      break;
  }
  if (!found)
    return vec3(0);
  vec2 e = vec2(.002, 0);
  vec3 n = normalize(vec3(face(hit + e.xyy) - face(hit - e.xyy), face(hit + e.yxy) - face(hit - e.yxy),
                          face(hit + e.yyx) - face(hit - e.yyx)));
  float weave = sin(hit.x * 38 + sin(hit.y * 19)) * sin(hit.y * 32 + sin(hit.z * 16));
  float emerge = .5 + .5 * sin(uTime * .16);
  float mask = smoothstep(-.8, .8, emerge * 2 - 1 + weave * .45);
  float light = .15 + .7 * max(0, dot(n, normalize(vec3(-.5, .8, 1))));
  float contour = line(weave, .06, max(fwidth(weave), .02));
  return palette(.64 + hit.y * .14) * (mask * light * .6 + contour * .25) * (1 + .08 * uPulse);
}
void main() {
  vec2 p = (vUv * 2 - 1) * vec2(uRes.x / uRes.y, 1);
  vec3 ro = uCam * (vec3(0, 0, 5) + uEye), rd = normalize(uCam * vec3(p, -2.1));
  vec3 c = vec3(0);
  if (uMode == 0)
    c = section(ro, rd);
  else if (uMode == 1)
    c = hypercube(ro, rd);
  else if (uMode == 2 || uMode == 3 || uMode == 6) {
    float t = -ro.z / rd.z;
    vec2 plane = (ro + rd * t).xy * (uMode == 2 ? .42 : .7);
    if (t > 0)
      c = uMode == 2 ? radial(plane) : (uMode == 3 ? folds(plane) : ornament(plane));
  } else if (uMode == 4)
    c = h3(p, 0);
  else if (uMode == 5) {
    float r = length(p), a = atan(p.y, p.x), aperture = .48 + .055 * sin(uTime * .7 + 7 * a);
    float f = smoothstep(aperture - .018, aperture + .018, r);
    vec3 outside = ornament(p);
    c = f >= 1 ? outside : mix(h3(p, 0), outside, f);
    c += vec3(.05, .3, .5) * line(r - aperture, .003, 2 / uRes.y);
  } else if (uMode == 7)
    c = events(ro, rd);
  else if (uMode == 8)
    c = emergence(ro, rd);
  else {
    float pane = floor(vUv.x * 3);
    vec2 q = vec2((fract(vUv.x * 3) * 2 - 1) * uRes.x / uRes.y / 3, p.y);
    c = h3(q, pane - 1);
    float border = min(fract(vUv.x * 3), 1 - fract(vUv.x * 3));
    c *= smoothstep(0, .015, border);
  }
  outColor = vec4(max(c, vec3(0)) * uExposure, 1);
}
)GLSL";
// HDR history is neighborhood-clamped so bright edges cannot leave long trails.
inline constexpr const char* temporal = R"GLSL(#version 410 core
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uCurrent, uHistory;
uniform vec2 uTexel;
uniform float uWeight;
void main() {
  vec3 c = texture(uCurrent, vUv).rgb;
  if (uWeight <= 0) {
    outColor = vec4(c, 1);
    return;
  }
  vec3 lo = c, hi = c;
  for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
      vec3 q = texture(uCurrent, vUv + vec2(x, y) * uTexel).rgb;
      lo = min(lo, q);
      hi = max(hi, q);
    }
  vec3 h = clamp(texture(uHistory, vUv).rgb, lo, hi);
  outColor = vec4(mix(c, h, uWeight), 1);
}
)GLSL";
inline std::string specialized(int mode) {
  if (mode < 0 || mode >= modes) throw std::out_of_range("atlas mode");
  std::string s = fragment;
  const std::string from = "uniform int uMode, uUseBvh, uEmpty;";
  s.replace(s.find(from), from.size(), "const int uMode=" + std::to_string(mode) + ";uniform int uUseBvh,uEmpty;");
  return s;
}
} // namespace dmt::gpu
