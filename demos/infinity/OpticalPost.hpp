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
uniform float uFade, uGlow, uUpscale;
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

void main() {
  vec3 c = uUpscale > .5 ? reconstruct(vUv) : texture(uImg, vUv).rgb;
  if (any(isnan(c)) || any(isinf(c))) c = vec3(0);
  c = clamp(c, vec3(0), vec3(60000));
  if (uGlow > 0) {
    vec3 halo = textureLod(uBloom, vUv, 1).rgb * .46 + textureLod(uBloom, vUv, 2).rgb * .29 +
                textureLod(uBloom, vUv, 3).rgb * .16 + textureLod(uBloom, vUv, 4).rgb * .09;
    c += halo * uGlow;
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
