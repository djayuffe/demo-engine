// Gradient-index ray lab, fully on the GPU.
//
// Design index (tuned for the design wavelength λ0 = 550 nm):
//   n(y) = 1 / ((1−k) + k·(y+ε)/(1+ε))
// At k = 0 it is uniform glass; as k → 1 it becomes n ∝ 1/y — the upper
// half-plane model of hyperbolic space, where light (Fermat's principle)
// travels along semicircles: hyperbolic geodesics.
//
// Dispersion: the medium is a blend of two plastics with Cauchy dispersion,
// so at another wavelength the index is an affine function of the design
// index, n_λ = n + D·s(λ)·(n − 1), s(λ) = (λ0/λ)² − 1. An affine (not merely
// scaled) index changes the ray curvature ∇n/n, so white light fans out into
// a rainbow — the broad-spectrum failure mode the article warns about.
//
// Pass 1 integrates  dr/ds = T,  dT/ds = (∇n − (∇n·T)T)/n  with RK4 into an
// RGBA32F texture (x = step, y = ray×wavelength). It runs in two stages so no
// texel re-integrates from the source: first a checkpoint every CHUNK steps,
// then each texel continues from its checkpoint (~10× less work than naive). Pass 2 paints the index
// field. Pass 3 draws every ray as an instanced screen-space ribbon that
// reads its vertices straight from that texture.

import { getGL, program, FULLSCREEN_VS, emptyVAO, fitCanvas, runWhenVisible } from './gl.js';

const STEPS = 512;
const CHUNK = 32; // integration is split: checkpoints every CHUNK steps, then short fills
const RAYS_MONO = 46;
const RAYS_WHITE = 26;
const NW = 7; // wavelengths sampled in white light
const ROWS = Math.max(RAYS_MONO, RAYS_WHITE * NW);
const EPS = 0.045;
const DISPERSION = 0.07;
export const SOURCE = { x: 0.055, y: 0.6 };

const COMMON = `
uniform float uK;
uniform float uAspect;
uniform int uRays;
uniform int uNW;
const float EPS = ${EPS.toFixed(4)};
const float DISP = ${DISPERSION.toFixed(4)};
float designIndex(vec2 r) {
  float y = clamp(r.y, -0.5 * EPS, 2.0);
  return 1.0 / ((1.0 - uK) + uK * (y + EPS) / (1.0 + EPS));
}
float lambdaOf(int w) {
  return uNW == 1 ? 550.0 : mix(420.0, 680.0, float(w) / float(uNW - 1));
}
float dispS(float lambda) { float q = 550.0 / lambda; return DISP * (q * q - 1.0); }
float indexAt(vec2 r, float ds) { float n = designIndex(r); return n + ds * (n - 1.0); }
vec2 gradN(vec2 r, float ds) {
  float n = designIndex(r);
  return vec2(0.0, -(1.0 + ds) * n * n * uK / (1.0 + EPS));
}`;

const INTEGRATE_FS = `#version 300 es
precision highp float;
precision highp int;
${COMMON}
uniform vec2 uSource;
uniform float uSpread;
uniform float uDs;
uniform int uPass;          // 0: checkpoints every CHUNK steps, 1: fill from checkpoints
uniform sampler2D uCk;
out vec4 outState;

float gDs;
vec4 deriv(vec4 s) {
  vec2 T = s.zw;
  vec2 g = gradN(s.xy, gDs);
  return vec4(T, (g - dot(g, T) * T) / indexAt(s.xy, gDs));
}

void main() {
  int col = int(gl_FragCoord.x);
  int row = int(gl_FragCoord.y);
  if (row >= uRays * uNW) { outState = vec4(-9.0); return; }
  int ray = row % uRays;
  gDs = dispS(lambdaOf(row / uRays));
  vec4 s;
  int n;
  if (uPass == 0) {
    float a = mix(-uSpread, uSpread, float(ray) / float(uRays - 1));
    s = vec4(uSource, cos(a), sin(a));
    n = col * ${CHUNK};
  } else {
    s = texelFetch(uCk, ivec2(col / ${CHUNK}, row), 0);
    n = col % ${CHUNK};
  }
  for (int i = 0; i < ${STEPS}; i++) {
    if (i >= n) break;
    vec4 k1 = deriv(s);
    vec4 k2 = deriv(s + 0.5 * uDs * k1);
    vec4 k3 = deriv(s + 0.5 * uDs * k2);
    vec4 k4 = deriv(s + uDs * k3);
    s += uDs / 6.0 * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
    s.zw = normalize(s.zw);
  }
  outState = s;
}`;

const FIELD_FS = `#version 300 es
precision highp float;
precision highp int;
${COMMON}
in vec2 vUv;
out vec4 outColor;
uniform vec2 uSource;
uniform vec2 uRes;
uniform vec3 uSourceColor;
void main() {
  vec2 r = vec2(vUv.x * uAspect, vUv.y);
  float n = designIndex(r);
  float ln = log(n);
  float lnMax = log(1.0 / ((1.0 - uK) + uK * EPS / (1.0 + EPS)));
  float x = clamp(ln / max(lnMax, 1e-3), 0.0, 1.0);
  vec3 col = mix(vec3(0.035, 0.06, 0.075), vec3(0.10, 0.075, 0.20), smoothstep(0.0, 1.0, x)) * (0.35 + 0.65 * uK);
  // Iso-index contours: the layers a printer would have to lay down.
  float lv = ln * 4.0;
  float f = abs(fract(lv) - 0.5);
  float w = fwidth(lv);
  col += vec3(0.35, 0.55, 0.75) * 0.06 * smoothstep(w * 1.2, 0.0, 0.5 - f) * uK;
  float d = length((vUv - uSource) * vec2(uAspect, 1.0)) * uRes.y;
  col += uSourceColor * (exp(-d * 0.22) * 1.2 + exp(-d * 0.035) * 0.08);
  outColor = vec4(col, 1.0);
}`;

const RAY_VS = `#version 300 es
precision highp float;
precision highp int;
${COMMON}
uniform sampler2D uState;
uniform vec2 uRes;
uniform float uWidth;
out float vSide;
out float vT;
out float vN;
out float vRay;
out float vLambda;

vec2 toPx(vec2 r) { return vec2(r.x / uAspect, r.y) * uRes; }

void main() {
  int i = gl_VertexID >> 1;
  float side = float(gl_VertexID & 1) * 2.0 - 1.0;
  int row = gl_InstanceID;
  vec4 s = texelFetch(uState, ivec2(i, row), 0);
  vec2 p = toPx(s.xy);
  vec2 t = normalize(vec2(s.z / uAspect, s.w) * uRes);
  vec2 px = p + vec2(-t.y, t.x) * side * uWidth;
  vSide = side;
  vT = float(i) / float(${STEPS - 1});
  vN = designIndex(s.xy);
  vRay = float(row % uRays) / float(uRays - 1);
  vLambda = lambdaOf(row / uRays);
  gl_Position = vec4(px / uRes * 2.0 - 1.0, 0.0, 1.0);
}`;

const RAY_FS = `#version 300 es
precision highp float;
precision highp int;
in float vSide;
in float vT;
in float vN;
in float vRay;
in float vLambda;
out vec4 outColor;
uniform float uTime;
uniform float uIntensity;
uniform float uSharp;
uniform float uNMax;
uniform int uNW;

// Smooth visible-spectrum → linear RGB (piecewise Gaussian fit).
vec3 spectrum(float l) {
  float r = exp(-pow((l - 610.0) / 55.0, 2.0)) + 0.35 * exp(-pow((l - 430.0) / 25.0, 2.0));
  float g = exp(-pow((l - 545.0) / 50.0, 2.0));
  float b = exp(-pow((l - 455.0) / 38.0, 2.0));
  return vec3(r, g, b);
}

void main() {
  float prof = exp(-vSide * vSide * uSharp);
  vec3 c;
  if (uNW == 1) {
    float x = clamp(log(vN) / max(log(uNMax), 1e-3), 0.0, 1.0);
    vec3 mint = vec3(0.50, 1.0, 0.92), blue = vec3(0.55, 0.68, 1.0), violet = vec3(0.80, 0.58, 1.0);
    c = mix(mix(mint, blue, smoothstep(0.0, 0.5, x)), violet, smoothstep(0.5, 1.0, x));
  } else {
    c = spectrum(vLambda) * (2.6 / float(uNW));
  }
  float phase = fract(uTime * 0.085 + vRay * 0.37);
  float dt = vT - phase;
  float pulse = exp(-dt * dt * 9000.0) * 5.0 + exp(-max(-dt, 0.0) * 30.0) * step(dt, 0.0) * 0.9;
  float fade = smoothstep(0.0, 0.03, vT) * (1.0 - smoothstep(0.8, 1.0, vT));
  float a = prof * uIntensity * fade * (0.22 + pulse);
  outColor = vec4(c * a, a);
}`;

export function createGrin(canvas) {
  const gl = getGL(canvas);
  if (!gl.getExtension('EXT_color_buffer_float')) throw new Error('Float render targets unavailable');

  const integ = program(gl, FULLSCREEN_VS, INTEGRATE_FS);
  const field = program(gl, FULLSCREEN_VS, FIELD_FS);
  const rays = program(gl, RAY_VS, RAY_FS);
  const vao = emptyVAO(gl);

  const tex = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, tex);
  gl.texStorage2D(gl.TEXTURE_2D, 1, gl.RGBA32F, STEPS, ROWS);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  const fbo = gl.createFramebuffer();
  gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
  gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);

  const ck = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, ck);
  gl.texStorage2D(gl.TEXTURE_2D, 1, gl.RGBA32F, STEPS / CHUNK, ROWS);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  const ckFbo = gl.createFramebuffer();
  gl.bindFramebuffer(gl.FRAMEBUFFER, ckFbo);
  gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, ck, 0);
  gl.bindFramebuffer(gl.FRAMEBUFFER, null);

  let target = 0.64, k = 0.64, white = false;
  let solved = '';

  const nMaxFor = (kk) => 1 / ((1 - kk) + kk * EPS / (1 + EPS));
  const layout = () => (white ? { rays: RAYS_WHITE, nw: NW } : { rays: RAYS_MONO, nw: 1 });

  function setCommon(p, aspect) {
    const { rays: r, nw } = layout();
    gl.uniform1f(p.u.uK, k);
    gl.uniform1f(p.u.uAspect, aspect);
    if (p.u.uRays) gl.uniform1i(p.u.uRays, r);
    if (p.u.uNW) gl.uniform1i(p.u.uNW, nw);
  }

  function integrate(aspect) {
    gl.disable(gl.BLEND);
    gl.useProgram(integ.program);
    setCommon(integ, aspect);
    gl.uniform2f(integ.u.uSource, SOURCE.x * aspect, SOURCE.y);
    gl.uniform1f(integ.u.uSpread, 1.22);
    gl.uniform1f(integ.u.uDs, (aspect * 1.7) / STEPS);
    gl.uniform1i(integ.u.uCk, 0);
    gl.activeTexture(gl.TEXTURE0);

    // Stage A: checkpoints (sampler bound to the other texture — no feedback loop).
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.bindFramebuffer(gl.FRAMEBUFFER, ckFbo);
    gl.viewport(0, 0, STEPS / CHUNK, ROWS);
    gl.uniform1i(integ.u.uPass, 0);
    gl.drawArrays(gl.TRIANGLES, 0, 3);

    // Stage B: every step continues from its checkpoint.
    gl.bindTexture(gl.TEXTURE_2D, ck);
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    gl.viewport(0, 0, STEPS, ROWS);
    gl.uniform1i(integ.u.uPass, 1);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
  }

  const loop = runWhenVisible(canvas, (t, dt) => {
    k += (target - k) * (1 - Math.exp(-dt * 7));
    if (Math.abs(target - k) < 1e-4) k = target;
    fitCanvas(canvas, 2.2e6);
    const W = canvas.width, H = canvas.height;
    const aspect = W / H;
    gl.bindVertexArray(vao);
    const key = `${k}|${aspect.toFixed(4)}|${white}`;
    if (key !== solved) { integrate(aspect); solved = key; }

    gl.viewport(0, 0, W, H);
    gl.disable(gl.BLEND);
    gl.useProgram(field.program);
    setCommon(field, aspect);
    gl.uniform2f(field.u.uSource, SOURCE.x, SOURCE.y);
    gl.uniform2f(field.u.uRes, W, H);
    gl.uniform3fv(field.u.uSourceColor, white ? [1, 0.97, 0.92] : [0.55, 1, 0.93]);
    gl.drawArrays(gl.TRIANGLES, 0, 3);

    gl.enable(gl.BLEND);
    gl.blendFunc(gl.ONE, gl.ONE);
    gl.useProgram(rays.program);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.uniform1i(rays.u.uState, 0);
    setCommon(rays, aspect);
    gl.uniform2f(rays.u.uRes, W, H);
    gl.uniform1f(rays.u.uTime, t);
    gl.uniform1f(rays.u.uNMax, nMaxFor(k));
    const { rays: r, nw } = layout();
    const dpr = H / canvas.clientHeight || 1;
    for (const [w, inten, sharp] of [[7 * dpr, 0.16, 2.2], [1.3 * dpr, 0.85, 1.6]]) {
      gl.uniform1f(rays.u.uWidth, w);
      gl.uniform1f(rays.u.uIntensity, inten);
      gl.uniform1f(rays.u.uSharp, sharp);
      gl.drawArraysInstanced(gl.TRIANGLE_STRIP, 0, STEPS * 2, r * nw);
    }
  });

  return {
    setCurvature(v) { target = v; loop.kick(); },
    setWhite(v) { white = v; loop.kick(); },
  };
}
