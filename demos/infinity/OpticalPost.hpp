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
uniform float uFade, uGlow, uRenderScale;
uniform vec2 uDrive;
// Four bilinear samples implement cubic B-spline reconstruction on the GPU.
// Positive weights avoid ringing around hot HDR cores and retain dark backgrounds.
vec3 reconstruct(vec2 uv){
  vec2 size=vec2(textureSize(uImg,0));
  vec2 p=uv*size-0.5, f=fract(p), base=floor(p);
  vec2 f2=f*f,f3=f2*f;
  vec2 w0=(1.0-3.0*f+3.0*f2-f3)/6.0;
  vec2 w1=(4.0-6.0*f2+3.0*f3)/6.0;
  vec2 w2=(1.0+3.0*f+3.0*f2-3.0*f3)/6.0;
  vec2 w3=f3/6.0;
  vec2 g0=w0+w1,g1=w2+w3;
  vec2 h0=(base-0.5+w1/g0)/size,h1=(base+1.5+w3/g1)/size;
  return texture(uImg,vec2(h0.x,h0.y)).rgb*g0.x*g0.y
       + texture(uImg,vec2(h1.x,h0.y)).rgb*g1.x*g0.y
       + texture(uImg,vec2(h0.x,h1.y)).rgb*g0.x*g1.y
       + texture(uImg,vec2(h1.x,h1.y)).rgb*g1.x*g1.y;
}

// Recover bounded local contrast after reconstruction. Clamping to the source
// cross prevents overshoot and dark/bright halos around thin HDR filaments.
vec3 restoreDetail(vec2 uv, vec3 center) {
  vec2 texel = 1.0 / vec2(textureSize(uImg, 0));
  vec3 left = texture(uImg, uv - vec2(texel.x, 0.0)).rgb;
  vec3 right = texture(uImg, uv + vec2(texel.x, 0.0)).rgb;
  vec3 down = texture(uImg, uv - vec2(0.0, texel.y)).rgb;
  vec3 up = texture(uImg, uv + vec2(0.0, texel.y)).rgb;
  vec3 localMin = min(center, min(min(left, right), min(down, up)));
  vec3 localMax = max(center, max(max(left, right), max(down, up)));
  vec3 average = (left + right + down + up) * 0.25;
  float range = max(localMax.r, max(localMax.g, localMax.b)) -
                min(localMin.r, min(localMin.g, localMin.b));
  float amount = 0.24 * smoothstep(0.003, 0.08, range);
  return clamp(center + (center - average) * amount, localMin, localMax);
}

// Nine bilinear taps integrate a supersampled source footprint before display.
// This turns spare GPU time into stable subpixel geometry instead of shimmer.
vec3 downsample(vec2 uv) {
  vec2 texel = 1.0 / vec2(textureSize(uImg, 0));
  vec2 offset = texel * clamp(uRenderScale * 0.34, 0.36, 0.56);
  vec3 center = texture(uImg, uv).rgb * 4.0;
  vec3 cardinal = texture(uImg, uv + vec2(offset.x, 0.0)).rgb +
                  texture(uImg, uv - vec2(offset.x, 0.0)).rgb +
                  texture(uImg, uv + vec2(0.0, offset.y)).rgb +
                  texture(uImg, uv - vec2(0.0, offset.y)).rgb;
  vec3 diagonal = texture(uImg, uv + offset).rgb + texture(uImg, uv - offset).rgb +
                  texture(uImg, uv + vec2(offset.x, -offset.y)).rgb +
                  texture(uImg, uv + vec2(-offset.x, offset.y)).rgb;
  return (center + cardinal * 2.0 + diagonal) * (1.0 / 16.0);
}

void main() {
  vec3 c = uRenderScale < .999 ? reconstruct(vUv) :
           uRenderScale > 1.001 ? downsample(vUv) : texture(uImg, vUv).rgb;
  if (uRenderScale < .999) c = restoreDetail(vUv, c);
  if (any(isnan(c)) || any(isinf(c))) c = vec3(0);
  c = clamp(c, vec3(0), vec3(60000));
  if (uGlow > 0) {
    vec3 halo = textureLod(uBloom, vUv, 1).rgb * .46 + textureLod(uBloom, vUv, 2).rgb * .29 +
                textureLod(uBloom, vUv, 3).rgb * .16 + textureLod(uBloom, vUv, 4).rgb * .09;
    c += halo * uGlow;
    vec2 bloomTexel = 1.0 / vec2(textureSize(uBloom, 0));
    float reach = 3.0 + 5.0 * uDrive.x;
    vec3 horizontal = textureLod(uBloom, vUv + vec2(bloomTexel.x * reach, 0.0), 1).rgb +
                      textureLod(uBloom, vUv - vec2(bloomTexel.x * reach, 0.0), 1).rgb;
    vec3 vertical = textureLod(uBloom, vUv + vec2(0.0, bloomTexel.y * reach), 1).rgb +
                    textureLod(uBloom, vUv - vec2(0.0, bloomTexel.y * reach), 1).rgb;
    vec3 diffraction = vec3(horizontal.r, (horizontal.g + vertical.g) * .5, vertical.b);
    c += diffraction * uGlow * (.018 + .032 * uDrive.y);
  }
  c *= 1.0 + .18 * uDrive.x + .45 * uDrive.y;
  c = pow(max(c - vec3(.008), vec3(0)), vec3(1.18));
  float peak = max(c.r, max(c.g, c.b));
  float mapped = peak <= .58 ? peak : .58 + .40 * (1 - exp(-(peak - .58) / .40));
  c *= mapped / max(peak, 1e-6);
  // Stationary triangular dither avoids 8-bit bands without pause-time sparkle.
  // Fade the noise near black so zero exposure and scene fades stay exactly black.
  vec2 q = gl_FragCoord.xy;
  float a = fract(52.9829189 * fract(dot(q, vec2(.06711056, .00583715))));
  float b = fract(52.9829189 * fract(dot(q + 37.7, vec2(.06711056, .00583715))));
  c += (a + b - 1.0) / 255.0 * smoothstep(0.0, .015, mapped);
  outColor = vec4(clamp(c, 0.0, 1.0) * uFade, 1);
}
)GLSL";
} // namespace infinity_post
