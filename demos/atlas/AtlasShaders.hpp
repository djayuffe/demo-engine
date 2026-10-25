#pragma once
#include <string>
#include <stdexcept>
namespace dmt::gpu {
inline constexpr int modes = 11;
// Sixteen projected corners are evaluated in the vertex stage, packed into
// 48 flat varying components: below the GL 4.1 minimum output budget.
inline constexpr const char* vertex=R"GLSL(#version 410 core
out vec2 vUv;
flat out vec4 vCorners[12];
uniform mat4 uRotation;
uniform float uBreath;
void main() {
  vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  vUv = p;
  gl_Position = vec4(p * 2 - 1, 0, 1);
  for (int i = 0; i < 12; i++)
    vCorners[i] = vec4(0);
  for (int i = 0; i < 16; i++) {
    vec4 v;
    for (int k = 0; k < 4; k++)
      v[k] = (i & (1 << k)) != 0 ? 1 : -1;
    v = uRotation * v * (1 + .025 * (2 * uBreath - 1));
    // The bounded tesseract has |w|<=2.05, entirely inside the 3.95 eye plane.
    vec3 projected = v.xyz * 4 / (4 - v.w);
    for (int k = 0; k < 3; k++) {
      int component = i * 3 + k;
      vCorners[component / 4][component % 4] = projected[k];
    }
  }
}
)GLSL";
inline constexpr const char* fragment = R"GLSL(#version 410 core
in vec2 vUv;
flat in vec4 vCorners[12];
uniform int uGpuCache;
layout(std140) uniform AtlasGeometry {
  vec4 uNodeCenters[112];
  vec4 uNodeData[112];
  vec4 uSphereCenters[56];
  vec4 uSphereData[56];
};
out vec4 outColor;
uniform vec2 uRes;
uniform vec3 uEye;
uniform mat3 uCam;
uniform mat4 uRotation;
uniform float uTime, uPulse, uBass, uExposure, uBreath;
uniform int uMode, uUseBvh, uEmpty;
uniform vec4 uPlanes[8], uBounds[112], uMeta[56], uLevels[9], uEvents[24];
const float PI = 3.14159265359;
vec3 palette(float t) { return .5 + .5 * cos(6.2831853 * (t + vec3(0, .33, .67))); }
mat2 rot(float a) { return mat2(cos(a), -sin(a), sin(a), cos(a)); }
float line(float d, float width, float aa) { return 1 - smoothstep(width, width + max(aa, .0001), abs(d)); }
float hash21(vec2 p) {
  vec3 q = fract(vec3(p.xyx) * .1031);
  q += dot(q, q.yzx + 33.33);
  return fract((q.x + q.y) * q.z);
}
vec3 starLayer(vec3 rd, float scale, float threshold) {
  vec2 sphereUv = vec2(atan(rd.z, rd.x) / (2 * PI), asin(clamp(rd.y, -1, 1)) / PI);
  vec2 cell = floor(sphereUv * scale), local = fract(sphereUv * scale) - .5;
  vec2 center = vec2(hash21(cell + 19.7), hash21(cell + 47.1)) - .5;
  center *= .62;
  float seed = hash21(cell), radius = mix(.018, .052, hash21(cell + 83.2));
  float star = (1 - smoothstep(radius, radius + .022, length(local - center))) * smoothstep(threshold, 1, seed);
  float sparkle = .70 + .30 * sin(uTime * mix(1.2, 3.8, seed) + seed * 41);
  vec3 temperature = mix(vec3(.25, .52, 1.25), vec3(1.30, .68, .30), hash21(cell + 121.9));
  return temperature * star * sparkle;
}
vec3 atlasBackground(vec2 p, vec3 rd) {
  vec3 direction = normalize(rd + uEye * .018);
  vec3 stars = starLayer(direction, 260, .955) + starLayer(direction.yzx, 470, .980) * .72;
  float longitude = atan(direction.z, direction.x), latitude = asin(clamp(direction.y, -1, 1));
  float fold = sin(longitude * 3.0 + sin(latitude * 5.0 - uTime * .055) * 1.4);
  float veil = exp(-pow(abs(latitude + .22 * fold), 2.0) * 19.0);
  float filaments = .36 + .64 * pow(.5 + .5 * sin(longitude * 17.0 - latitude * 31.0 + uTime * .12), 6.0);
  vec3 nebula = mix(vec3(.015, .055, .16), vec3(.16, .025, .24), .5 + .5 * fold) * veil * filaments;
  float radius = length(p);
  float beatWave = exp(-abs(radius - fract(uTime * .16) * 1.8) * 24.0) * uPulse;
  vec3 wave = palette(.57 + radius * .08 + uTime * .006) * beatWave * .055;
  float vignette = 1.0 - smoothstep(.72, 1.75, radius);
  return (stars * (.24 + .26 * uBass) + nebula * (1.0 + .28 * uBreath) + wave) * (.34 + .66 * vignette);
}
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
vec3 projectedCorner(int i) {
  int k = i * 3;
  return vec3(vCorners[k / 4][k % 4], vCorners[(k + 1) / 4][(k + 1) % 4], vCorners[(k + 2) / 4][(k + 2) % 4]);
}
vec3 hypercube(vec3 ro, vec3 rd) {
  vec3 color = vec3(0);
  for (int i = 0; i < 16; i++)
    for (int axis = 0; axis < 4; axis++) {
      int j = i ^ (1 << axis);
      if (i >= j)
        continue;
      vec3 pa, pb;
      if (uGpuCache != 0) {
        pa = projectedCorner(i);
        pb = projectedCorner(j);
      } else {
        vec4 a, b;
        for (int k = 0; k < 4; k++) {
          a[k] = (i & (1 << k)) != 0 ? 1 : -1;
          b[k] = (j & (1 << k)) != 0 ? 1 : -1;
        }
        a = uRotation * a * (1 + .025 * (2 * uBreath - 1));
        b = uRotation * b * (1 + .025 * (2 * uBreath - 1));
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
        pa = a.xyz * 4 / (4 - a.w);
        pb = b.xyz * 4 / (4 - b.w);
      }
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
vec3 radial(vec2 p, float pixel) {
  float r = max(length(p), .001), a = atan(p.y, p.x), sum = 0;
  for (int k = 0; k < 9; k++) {
    vec4 l = uLevels[k];
    float phase = l.x * a + uTime * (.25 + float(k) * .04) + l.y + l.z * log(r);
    float footprint = length(vec2(l.x, l.z)) * pixel / r;
    float lod = 1 - smoothstep(.25, 1.5, footprint);
    sum += l.w * sin(phase) * lod;
  }
  float ridge = pow(.5 + .5 * cos(sum * 4), 4);
  float iris = exp(-r * r * .6);
  return palette(.62 + sum * .10 + uTime * .02) * (ridge * .85 + abs(tanh(sum * 1.5)) * .18) * iris *
         (.85 + .12 * uPulse);
}
vec3 folds(vec2 p, float pixel) {
  float accum = 0;
  float footprint = pixel;
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
float hitSphere(vec4 o, vec4 d, vec4 c, float C, bool bound) {
  float A = -lor(o, c), B = -lor(d, c);
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
float sphere(vec4 o, vec4 d, vec4 packed, bool bound) {
  return hitSphere(o, d, lift(packed.xyz), cosh(packed.w), bound);
}
vec4 primitive(int i) {
  float f = float(i), phi = f * 2.39996323;
  return vec4(.92 * cos(phi), .92 * sin(phi), 1.7 + float(i % 8) * .44,
              (.16 + .035 * float(i % 4)) * (1 + .025 * uBreath));
}
void sphereShape(int i, out vec4 center, out vec3 shape) {
  if (uGpuCache != 0) {
    center = uSphereCenters[i];
    vec3 base = uSphereData[i].xyz;
    float delta = base.x * .025 * uBreath, c = 1 + delta * delta * .5, s = delta * (1 + delta * delta / 6);
    shape = vec3(base.x + delta, base.y * c + base.z * s, base.z * c + base.y * s);
  } else {
    vec4 p = primitive(i);
    center = lift(p.xyz);
    shape = vec3(p.w, cosh(p.w), sinh(p.w));
  }
}
float primitiveHit(vec4 o, vec4 d, int i) {
  vec4 center;
  vec3 shape;
  sphereShape(i, center, shape);
  return hitSphere(o, d, center, shape.y, false);
}
// The same exact Lorentz traversal serves primary and finite point-light rays.
float traceH3(vec4 o, vec4 d, float limit, bool shadow, out int material) {
  float nearest = limit;
  material = -1;
  if (uUseBvh != 0) {
    int index = 0;
    for (int iteration = 0; iteration < 112; iteration++) {
      if (index >= 111)
        break;
      vec2 info;
      float entry;
      if (uGpuCache != 0) {
        vec4 data = uNodeData[index];
        info = data.yz;
        entry = hitSphere(o, d, uNodeCenters[index], data.x, true);
      } else {
        vec4 meta = uMeta[index / 2];
        info = (index % 2) == 0 ? meta.xy : meta.zw;
        entry = sphere(o, d, uBounds[index], true);
      }
      if (entry >= 1e9 || entry > nearest) {
        index = int(info.x + .5);
        continue;
      }
      int leaf = int(info.y + .5) - 1;
      if (leaf >= 0) {
        float t = primitiveHit(o, d, leaf);
        if (t < nearest) {
          nearest = t;
          material = leaf;
          if (shadow)
            return nearest;
        }
      }
      index++;
    }
  } else
    for (int i = 0; i < 56; i++) {
      float t = primitiveHit(o, d, i);
      if (t < nearest) {
        nearest = t;
        material = i;
        if (shadow)
          return nearest;
      }
    }
  return nearest;
}
vec3 microfacet(vec3 albedo, vec3 n, vec3 v, vec3 l, float roughness) {
  float nv = clamp(dot(n, v), 0, 1), nl = clamp(dot(n, l), 0, 1);
  vec3 halfv = v + l;
  vec3 h = halfv * inversesqrt(max(dot(halfv, halfv), 1e-8));
  float nh = clamp(dot(n, h), 0, 1), vh = clamp(dot(v, h), 0, 1), a = roughness * roughness, a2 = a * a;
  float denominator = nh * nh * (a2 - 1) + 1;
  float distribution = a2 / (PI * denominator * denominator);
  float gv = 2 * nv / max(nv + sqrt(a2 + (1 - a2) * nv * nv), 1e-5);
  float gl = 2 * nl / max(nl + sqrt(a2 + (1 - a2) * nl * nl), 1e-5);
  vec3 fresnel = vec3(.045) + vec3(.955) * pow(1 - vh, 5);
  vec3 spec = distribution * gv * gl * fresnel / max(4 * nv * nl, 1e-5);
  return ((1 - fresnel) * albedo / PI + spec) * nl;
}
vec3 restFrame(vec4 p, vec4 v) {
  vec3 q = v.yzw - p.yzw * v.x / (p.x + 1);
  return q * inversesqrt(max(dot(q, q), 1e-10));
}
vec3 h3(vec2 p, float view) {
  float turn = uTime * .09;
  vec3 origin = uCam * (uEye * .22 + vec3(view * .38, 0, 0));
  vec3 direction = normalize(uCam * vec3(p * .92, 1.25));
  origin.xy = rot(turn) * origin.xy;
  direction.xy = rot(turn) * direction.xy;
  vec4 o = lift(origin), d = tangent(o, direction);
  int material;
  float nearest = traceH3(o, d, 1e10, false, material);
  if (material < 0) {
    float r = length(p), a = atan(p.y, p.x);
    float ridge = pow(abs(sin(15 * a + 8 * log(r + .02) - uTime)), 18);
    float coverage = 1 - smoothstep(.2, 1, 30 / (max(r, .01) * uRes.y));
    return vec3(.02, .13, .25) * ridge * coverage;
  }
  vec4 center;
  vec3 shape;
  sphereShape(material, center, shape);
  vec4 hit = o * cosh(nearest) + d * sinh(nearest);
  vec4 n = (shape.y * hit - center) / shape.z;
  n /= sqrt(max(lor(n, n), 1e-8));
  vec3 normal = restFrame(hit, n);
  vec4 viewT = -(o * sinh(nearest) + d * cosh(nearest));
  vec3 v = restFrame(hit, viewT);
  vec4 lamp = lift(vec3(-1.4, 2.1, .2));
  float coshDistance = max(1.00001, -lor(hit, lamp));
  vec4 lightT = (lamp - coshDistance * hit) / sqrt(coshDistance * coshDistance - 1);
  vec3 light = restFrame(hit, lightT);
  float visibility = 1;
  if (dot(normal, light) > 0) {
    // Offset along the surface geodesic, not in Poincare screen coordinates.
    vec4 offset = hit * cosh(.004) + n * sinh(.004);
    float A = max(1.00001, -lor(offset, lamp)), root = sqrt(A * A - 1);
    vec4 toward = (lamp - A * offset) / root;
    int blocker;
    traceH3(offset, toward, log(A + root) - .006, true, blocker);
    if (blocker >= 0)
      visibility = 0;
  }
  float azimuth = max(length(normal.xy), .06);
  float footprint = (22 / azimuth + 14) * sinh(nearest) * 1.84 / (1.25 * uRes.y * shape.z);
  float bands =
      .5 + .5 * sin(22 * atan(normal.y, normal.x) + uTime + 14 * normal.z) * exp(-footprint * footprint / 24);
  vec3 color = mix(vec3(.08, .55, .95), vec3(.78, .13, .44), bands);
  float roughness = mix(.26, .40, bands);
  vec3 direct = microfacet(color, normal, v, light, roughness);
  float attenuation = 6.0 / (1 + .04 * (coshDistance * coshDistance - 1));
  float rim = pow(1 - max(dot(normal, v), 0), 4);
  return (direct * visibility * attenuation + color * .035 + vec3(.025, .08, .12) * rim) *
         exp(-nearest * .07) * (1 + .10 * uPulse + .06 * uBass);
}
vec3 ornament(vec2 p, float pixel) {
  float r = length(p), a = atan(p.y, p.x);
  float angle = 14 * a + uTime + 3 * log(r + .01), rad = 24 * r - uTime * .4;
  float footprint = (14 / max(r, .01) + 24) * pixel;
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
      // Authored node time is arrival time: packets depart at the parent's
      // event and reach this child exactly when its light activates.
      float progress = clamp((clock - parent.w) / max(e.w - parent.w, .001), 0, 1);
      vec3 end = mix(parent.xyz, e.xyz, progress);
      d = segment(ro, rd, parent.xyz, end, depth);
      if (depth > 0)
        color += tint * line(d, .007, depth / uRes.y) * life * .65;
      float travelling =
          smoothstep(parent.w, parent.w + .12, clock) * (1 - smoothstep(e.w - .12, e.w, clock));
      d = segment(ro, rd, end, end + vec3(0, .001, 0), depth);
      if (depth > 0)
        color += tint * exp(-d * d / .002) * travelling * 1.25;
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
  float b = dot(ro, rd), disc = b * b - dot(ro, ro) + 2.25;
  if (disc < 0)
    return vec3(0);
  float root = sqrt(disc), t = max(0, -b - root), end = -b + root;
  if (end <= 0)
    return vec3(0);
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
    if (t > end)
      break;
  }
  if (!found)
    return vec3(0);
  vec2 e = vec2(.002, 0);
  vec3 gradient = vec3(face(hit + e.xyy) - face(hit - e.xyy), face(hit + e.yxy) - face(hit - e.yxy),
                       face(hit + e.yyx) - face(hit - e.yyx));
  vec3 n = gradient * inversesqrt(max(dot(gradient, gradient), 1e-10));
  float weave = sin(hit.x * 38 + sin(hit.y * 19)) * sin(hit.y * 32 + sin(hit.z * 16));
  float emerge = .5 + .5 * sin(uTime * .16);
  float mask = smoothstep(-.8, .8, emerge * 2 - 1 + weave * .45);
  float light = .15 + .7 * max(0, dot(n, normalize(vec3(-.5, .8, 1))));
  // Analytic material footprint remains defined across divergent ray marches.
  float wx = hit.x * 38 + sin(hit.y * 19), wy = hit.y * 32 + sin(hit.z * 16);
  vec3 weaveGradient =
      vec3(38 * cos(wx) * sin(wy), 19 * cos(hit.y * 19) * cos(wx) * sin(wy) + 32 * sin(wx) * cos(wy),
           16 * cos(hit.z * 16) * sin(wx) * cos(wy));
  float footprint = length(weaveGradient) * t * 2 / (2.1 * uRes.y * max(abs(dot(n, rd)), .15));
  float contour = line(weave, .06, max(footprint, .02));
  float ao = 1;
  for (int i = 1; i <= 3; i++) {
    float distance = .04 * float(i);
    ao -= max(0, distance - face(hit + n * distance)) * (.16 / distance);
  }
  light *= clamp(ao, .35, 1);
  return palette(.64 + hit.y * .14) * (mask * light * .6 + contour * .25) * (1 + .08 * uPulse);
}
vec4 roomPlane(int face) {
  vec4 normal = vec4(sinh(.52), 0, 0, 0);
  normal[1 + face / 2] = (face % 2 == 0 ? 1 : -1) * cosh(.52);
  return normal;
}
float planeHit(vec4 origin, vec4 direction, vec4 normal) {
  float denominator = lor(direction, normal);
  if (abs(denominator) < 1e-7)
    return 1e10;
  float ratio = -lor(origin, normal) / denominator;
  if (ratio <= 1e-6 || ratio >= 1)
    return 1e10;
  return atanh(ratio);
}
vec3 mirrorGallery(vec2 p) {
  vec4 origin = lift(uCam * (uEye * .22)), direction = tangent(origin, normalize(uCam * vec3(p * .92, 1.25)));
  vec3 result = vec3(0), throughput = vec3(1);
  int previous = -1;
  vec4 center = lift(vec3(.06, .025, .32));
  float radius = .065 * (1 + .025 * uBreath);
  for (int bounce = 0; bounce < 6; bounce++) {
    int side = -1;
    float nearest = 1e10;
    for (int face = 0; face < 6; face++) {
      if (face == previous)
        continue;
      float t = planeHit(origin, direction, roomPlane(face));
      if (t < nearest) {
        nearest = t;
        side = face;
      }
    }
    float ball = hitSphere(origin, direction, center, cosh(radius), false);
    if (ball < nearest) {
      vec4 hit = origin * cosh(ball) + direction * sinh(ball);
      vec4 normal = (cosh(radius) * hit - center) / sinh(radius);
      vec3 n = restFrame(hit, normal), view = restFrame(hit, -(origin * sinh(ball) + direction * cosh(ball)));
      vec4 lamp = lift(vec3(-.22, .26, .05));
      float A = max(1.00001, -lor(hit, lamp));
      vec3 light = restFrame(hit, (lamp - A * hit) / sqrt(A * A - 1));
      vec3 albedo = palette(.62 + normal.z * .15);
      result +=
          throughput * (microfacet(albedo, n, view, light, .28) * 1.8 + albedo * .045) * (1 + .1 * uPulse);
      break;
    }
    if (side < 0 || nearest > 20)
      break;
    vec4 hit = origin * cosh(nearest) + direction * sinh(nearest),
         travel = origin * sinh(nearest) + direction * cosh(nearest);
    vec4 normal = roomPlane(side);
    // Inverse boost maps this geodesic wall to an H2 plane through the origin.
    int axis = side / 2;
    float signSide = side % 2 == 0 ? 1 : -1;
    float localTime = cosh(.52) * hit.x - signSide * sinh(.52) * hit[axis + 1];
    vec2 coordinates = vec2(hit[1 + (axis + 1) % 3], hit[1 + (axis + 2) % 3]);
    float crossDistance = min(abs(asinh(coordinates.x)), abs(asinh(coordinates.y)));
    float rings = sin(acosh(max(localTime, 1)) * 18 - uTime * .15);
    float footprint = (1 + float(bounce)) * 2 / uRes.y;
    float mark = line(crossDistance, .004, footprint) * .65 + line(rings, .045, footprint * 18) * .2;
    float edge = 100;
    for (int face = 0; face < 6; face++)
      if (face != side)
        edge = min(edge, abs(lor(hit, roomPlane(face))));
    vec3 tint = palette(float(side) * .14 + uTime * .008);
    result += throughput * tint * (mark * .25 + exp(-edge * 80) * .3);
    throughput *= mix(vec3(.78), tint, .08);
    travel -= 2 * lor(travel, normal) * normal;
    // Reproject after each reflection to contain finite-precision drift.
    origin = lift(hit.yzw);
    travel += lor(origin, travel) * origin;
    direction = travel * inversesqrt(max(lor(travel, travel), 1e-8));
    previous = side;
  }
  return result;
}
float relief(vec2 p) {
  float r = length(p);
  if (uMode == 3)
    return .06 * sin(p.x * 3 + uTime * .15) * cos(p.y * 3 - uTime * .1);
  return (.065 + .015 * uBreath) * sin(r * 5 - uTime * .35) * exp(-r * r * .4);
}
vec3 reliefSurface(vec3 ro, vec3 rd, vec3 rayDx, vec3 rayDy) {
  if (abs(rd.z) < 1e-5)
    return vec3(0);
  float a = (-.1 - ro.z) / rd.z, b = (.1 - ro.z) / rd.z, lo = min(a, b), hi = max(a, b);
  if (hi <= 0)
    return vec3(0);
  float t;
  if (abs(rd.z) > .5) {
    // |grad(height)| < .5, so this bracket is monotone at ordinary view angles.
    float signLo = rd.z < 0 ? 1 : -1;
    for (int i = 0; i < 11; i++) {
      float mid = (lo + hi) * .5;
      vec3 q = ro + rd * mid;
      float f = q.z - relief(q.xy);
      if (f * signLo > 0)
        lo = mid;
      else
        hi = mid;
    }
    t = (lo + hi) * .5;
  } else {
    // At grazing angles use a Lipschitz step to retain the first intersection.
    t = max(0, lo);
    bool found = false;
    float bound = abs(rd.z) + .5 * length(rd.xy);
    for (int i = 0; i < 96; i++) {
      vec3 q = ro + rd * t;
      float distance = abs(q.z - relief(q.xy));
      if (distance < .00015) {
        found = true;
        break;
      }
      t += distance / max(bound, 1e-5);
      if (t > min(hi, 40))
        break;
    }
    if (!found)
      return vec3(0);
  }
  vec3 hit = ro + rd * t;
  vec2 plane = hit.xy * (uMode == 2 ? .42 : .7);
  if (t <= 0)
    return vec3(0);
  float e = .002;
  vec3 n = normalize(vec3(relief(hit.xy - vec2(e, 0)) - relief(hit.xy + vec2(e, 0)),
                          relief(hit.xy - vec2(0, e)) - relief(hit.xy + vec2(0, e)), 2 * e));
  float denominator = dot(n, rd);
  denominator = denominator < 0 ? min(denominator, -1e-4) : max(denominator, 1e-4);
  vec3 surfaceDx = t * (rayDx - rd * dot(n, rayDx) / denominator),
       surfaceDy = t * (rayDy - rd * dot(n, rayDy) / denominator);
  float pixel = max(length(surfaceDx.xy), length(surfaceDy.xy)) * (uMode == 2 ? .42 : .7);
  vec3 color =
      uMode == 2 ? radial(plane, pixel) : (uMode == 3 ? folds(plane, pixel) : ornament(plane, pixel));
  float light = .8 + .2 * max(dot(n, normalize(vec3(-.5, .8, 1))), 0);
  return color * light * (.97 + .06 * uBreath + .04 * uBass);
}
void main() {
  vec2 p = (vUv * 2 - 1) * vec2(uRes.x / uRes.y, 1);
  vec3 ro = uCam * (vec3(0, 0, 5) + uEye), rd = normalize(uCam * vec3(p, -2.1));
  vec3 rayDx = dFdx(rd), rayDy = dFdy(rd);
  vec3 c = atlasBackground(p, rd);
  if(uMode==-1){outColor=vec4(max(c,vec3(0))*uExposure,1);return;}
  if (uMode == 0)
    c += section(ro, rd);
  else if (uMode == 1)
    c += hypercube(ro, rd);
  else if (uMode == 2 || uMode == 3 || uMode == 6)
    c += reliefSurface(ro, rd, rayDx, rayDy);
  else if (uMode == 4)
    c += h3(p, 0);
  else if (uMode == 5) {
    float r = length(p), a = atan(p.y, p.x), aperture = .48 + .055 * sin(uTime * .7 + 7 * a);
    float f = smoothstep(aperture - .018, aperture + .018, r);
    vec3 outside = ornament(p, 2 / uRes.y);
    c += f >= 1 ? outside : mix(h3(p, 0), outside, f);
    c += vec3(.05, .3, .5) * line(r - aperture, .003, 2 / uRes.y);
  } else if (uMode == 7)
    c += events(ro, rd);
  else if (uMode == 8)
    c += emergence(ro, rd);
  else if(uMode==9) {
    float pane = floor(vUv.x * 3);
    vec2 q = vec2((fract(vUv.x * 3) * 2 - 1) * uRes.x / uRes.y / 3, p.y);
    vec3 panes = h3(q, pane - 1);
    float border = min(fract(vUv.x * 3), 1 - fract(vUv.x * 3));
    c += panes * smoothstep(0, .015, border);
  } else c=mirrorGallery(p);
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
inline std::string specialized(int mode,bool cached=true) {
  if (mode < 0 || mode >= modes) throw std::out_of_range("atlas mode");
  std::string s = fragment;
  const std::string from = "uniform int uMode, uUseBvh, uEmpty;";
  s.replace(s.find(from), from.size(), "const int uMode=" + std::to_string(mode) + ";uniform int uUseBvh,uEmpty;");
  const std::string cache="uniform int uGpuCache;";
  s.replace(s.find(cache),cache.size(),"const int uGpuCache="+std::to_string(cached?1:0)+";");
  return s;
}
inline std::string background(){auto source=specialized(0);const std::string mode="const int uMode=0;";source.replace(source.find(mode),mode.size(),"const int uMode=-1;");return source;}
} // namespace dmt::gpu
