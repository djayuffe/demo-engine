#pragma once
namespace infinity_post {
// Four-tap downsample and soft-knee extraction; black input remains exactly black.
inline constexpr const char* extract = R"GLSL(#version 410 core
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uImg;
uniform vec2 uTexel;
vec3 highlight(vec3 c) {
  float peak = max(c.r, max(c.g, c.b));
  float soft = clamp(peak - .32, 0, .32);
  soft = soft * soft / .64;
  float weight = max(peak - .48, soft) / max(peak, 1e-6);
  return c * weight;
}
void main() {
  vec3 sum = vec3(0);
  float weight = 0;
  for (int y = 0; y < 2; y++)
    for (int x = 0; x < 2; x++) {
      vec3 c = highlight(max(texture(uImg, vUv + (vec2(x, y) - .5) * uTexel).rgb, vec3(0)));
      float w = 1 / (1 + max(c.r, max(c.g, c.b)));
      sum += c * w;
      weight += w;
    }
  outColor = vec4(sum / max(weight, 1e-6), 1);
}
)GLSL";
inline constexpr const char* composite = R"GLSL(#version 410 core
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uImg, uBloom;
uniform float uFade, uGlow;
void main() {
  vec3 c = max(texture(uImg, vUv).rgb, vec3(0));
  if (uGlow > 0) {
    vec3 halo = textureLod(uBloom, vUv, 1).rgb * .46 + textureLod(uBloom, vUv, 2).rgb * .29 +
                textureLod(uBloom, vUv, 3).rgb * .16 + textureLod(uBloom, vUv, 4).rgb * .09;
    c += halo * uGlow;
  }
  c = pow(max(c - vec3(.008), vec3(0)), vec3(1.18));
  float peak = max(c.r, max(c.g, c.b));
  float mapped = peak <= .58 ? peak : .58 + .40 * (1 - exp(-(peak - .58) / .40));
  c *= mapped / max(peak, 1e-6);
  outColor = vec4(c * uFade, 1);
}
)GLSL";
} // namespace infinity_post
