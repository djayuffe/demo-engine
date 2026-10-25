#pragma once
#include "AtlasShaders.hpp"
namespace dmt::gpu {
inline constexpr const char* lineVertex = R"GLSL(#version 410 core
flat out int edgeIndex;
void main() {
  edgeIndex = gl_InstanceID;
  gl_Position = vec4(0, 0, 0, 1);
}
)GLSL";
inline constexpr const char* lineGeometry = R"GLSL(#version 410 core
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
flat in int edgeIndex[];
flat out vec3 vA, vB;
flat out float vAxis;
uniform mat4 uRotation;
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec2 uRes;
uniform float uBreath;
void main() {
  int axis = edgeIndex[0] / 8, bits = edgeIndex[0] % 8, id = 0, sourceBit = 0;
  for (int bit = 0; bit < 4; bit++)
    if (bit != axis) {
      id |= ((bits >> sourceBit) & 1) << bit;
      sourceBit++;
    }
  vec4 a, b;
  for (int k = 0; k < 4; k++) {
    a[k] = (id & (1 << k)) != 0 ? 1 : -1;
    b[k] = ((id ^ (1 << axis)) & (1 << k)) != 0 ? 1 : -1;
  }
  float scale = 1 + .025 * (2 * uBreath - 1);
  a = uRotation * a * scale;
  b = uRotation * b * scale;
  vec3 first = a.xyz * 4 / (4 - a.w), second = b.xyz * 4 / (4 - b.w);
  vec3 ca = transpose(uCam) * first - vec3(0, 0, 5) - uEye,
       cb = transpose(uCam) * second - vec3(0, 0, 5) - uEye;
  vec2 lo = vec2(1e6), hi = vec2(-1e6), aspect = vec2(uRes.x / uRes.y, 1);
  const float radius = .28;
  // A capsule lies inside the convex hull of these endpoint boxes. Perspective
  // extrema occur at their vertices while all depths remain ahead of the eye.
  bool safe = true;
  for (int endpoint = 0; endpoint < 2; endpoint++)
    for (int k = 0; k < 8; k++) {
      vec3 p = endpoint == 0 ? ca : cb;
      p += radius * vec3((k & 1) != 0 ? 1 : -1, (k & 2) != 0 ? 1 : -1, (k & 4) != 0 ? 1 : -1);
      if (p.z >= -.01) {
        safe = false;
        continue;
      }
      vec2 screen = p.xy * 2.1 / (-p.z) / aspect;
      lo = min(lo, screen);
      hi = max(hi, screen);
    }
  if (!safe) {
    lo = vec2(-1);
    hi = vec2(1);
  } // Conservative fallback for future camera ranges.
  lo = max(lo, vec2(-1));
  hi = min(hi, vec2(1));
  if (any(greaterThan(lo, hi)))
    return;
  for (int k = 0; k < 4; k++) {
    vA = first;
    vB = second;
    vAxis = float(axis);
    gl_Position = vec4((k & 1) != 0 ? hi.x : lo.x, (k & 2) != 0 ? hi.y : lo.y, 0, 1);
    EmitVertex();
  }
  EndPrimitive();
}
)GLSL";
inline std::string lineFragment() {
  std::string source = fragment;
  const auto begin = source.find("vec3 palette("), end = source.find("vec3 section(");
  if (begin == std::string::npos || end == std::string::npos)
    throw std::runtime_error("line shader helper contract changed");
  return std::string(R"GLSL(#version 410 core
flat in vec3 vA, vB;
flat in float vAxis;
out vec4 outColor;
uniform vec2 uRes;
uniform vec3 uEye;
uniform mat3 uCam;
uniform float uTime, uExposure;
)GLSL") + source.substr(begin, end - begin) +
         R"GLSL(
void main() {
  vec2 p = (gl_FragCoord.xy / uRes * 2 - 1) * vec2(uRes.x / uRes.y, 1);
  vec3 ro = uCam * (vec3(0, 0, 5) + uEye), rd = normalize(uCam * vec3(p, -2.1));
  float depth;
  float d = segment(ro, rd, vA, vB, depth);
  if (depth <= 0 || d > .28)
    discard;
  float core = line(d, .008, depth / uRes.y);
  vec3 color =
      palette(vAxis * .21 + uTime * .015) * (core * 1.3 + .12 * exp(-d * 35)) / (1 + .04 * depth * depth);
  outColor = vec4(color * uExposure, 0);
}
)GLSL";
}
} // namespace dmt::gpu
