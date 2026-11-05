// Small WebGL2 toolkit shared by the three GPU figures.

export function getGL(canvas, opts = {}) {
  const gl = canvas.getContext('webgl2', {
    antialias: false,
    alpha: true,
    premultipliedAlpha: true,
    preserveDrawingBuffer: false,
    powerPreference: 'high-performance',
    ...opts,
  });
  if (!gl) throw new Error('WebGL2 unavailable');
  return gl;
}

function compile(gl, type, src) {
  const sh = gl.createShader(type);
  gl.shaderSource(sh, src);
  gl.compileShader(sh);
  if (!gl.getShaderParameter(sh, gl.COMPILE_STATUS)) {
    const log = gl.getShaderInfoLog(sh);
    const numbered = src.split('\n').map((l, i) => `${i + 1}: ${l}`).join('\n');
    gl.deleteShader(sh);
    throw new Error(`Shader compile failed:\n${log}\n${numbered}`);
  }
  return sh;
}

// Builds a program and returns { program, u } where u is a cache of uniform locations.
export function program(gl, vsSrc, fsSrc) {
  const p = gl.createProgram();
  gl.attachShader(p, compile(gl, gl.VERTEX_SHADER, vsSrc));
  gl.attachShader(p, compile(gl, gl.FRAGMENT_SHADER, fsSrc));
  gl.linkProgram(p);
  if (!gl.getProgramParameter(p, gl.LINK_STATUS)) {
    throw new Error(`Program link failed: ${gl.getProgramInfoLog(p)}`);
  }
  const u = {};
  const n = gl.getProgramParameter(p, gl.ACTIVE_UNIFORMS);
  for (let i = 0; i < n; i++) {
    const info = gl.getActiveUniform(p, i);
    const name = info.name.replace(/\[0\]$/, '');
    u[name] = gl.getUniformLocation(p, info.name);
  }
  return { program: p, u };
}

// Fullscreen triangle generated from gl_VertexID — no buffers needed.
export const FULLSCREEN_VS = `#version 300 es
out vec2 vUv;
void main() {
  vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  vUv = p;
  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}`;

export function emptyVAO(gl) {
  return gl.createVertexArray();
}

// Resize the drawing buffer to the element's CSS size × DPR, capped to a pixel budget.
export function fitCanvas(canvas, maxPixels = 1.6e6, maxDpr = 2) {
  const rect = canvas.getBoundingClientRect();
  let dpr = Math.min(window.devicePixelRatio || 1, maxDpr);
  const area = rect.width * rect.height * dpr * dpr;
  if (area > maxPixels) dpr *= Math.sqrt(maxPixels / area);
  const w = Math.max(1, Math.round(rect.width * dpr));
  const h = Math.max(1, Math.round(rect.height * dpr));
  if (canvas.width !== w || canvas.height !== h) {
    canvas.width = w;
    canvas.height = h;
    return true;
  }
  return false;
}

// Runs `frame(t, dt)` only while the canvas is on-screen and the tab is visible.
export function runWhenVisible(canvas, frame) {
  let visible = false;
  let raf = 0;
  let last = 0;
  let clock = 0;
  const reduce = window.matchMedia('(prefers-reduced-motion: reduce)');

  const tick = (now) => {
    raf = 0;
    if (!visible || document.hidden) return;
    const dt = last ? Math.min((now - last) / 1000, 0.1) : 0;
    last = now;
    clock += reduce.matches ? 0 : dt;
    // A frame may return false to go idle until kick() is called.
    if (frame(clock, dt) === false) return;
    raf = requestAnimationFrame(tick);
  };
  const start = () => {
    if (!raf && visible && !document.hidden) {
      last = 0;
      raf = requestAnimationFrame(tick);
    }
  };

  new IntersectionObserver((entries) => {
    visible = entries.some((e) => e.isIntersecting);
    start();
  }, { rootMargin: '120px' }).observe(canvas);
  document.addEventListener('visibilitychange', start);

  return { kick: start };
}

// Adaptive pixel budget: shrinks the render resolution when frames run long
// and grows it back when there is headroom. Call update(dt) once per frame.
export function adaptiveBudget(base, { min = 0.3, slow = 0.026, fast = 0.0185 } = {}) {
  let scale = 1, ema = 1 / 60, wait = 0;
  return {
    update(dt) {
      if (!(dt > 0)) return base * scale;
      ema += (dt - ema) * 0.08;
      wait -= dt;
      if (wait <= 0) {
        if (ema > slow && scale > min) { scale = Math.max(min, scale * 0.82); wait = 0.45; }
        else if (ema < fast && scale < 1) { scale = Math.min(1, scale * 1.06); wait = 0.6; }
      }
      return base * scale;
    },
    get scale() { return scale; },
  };
}

// Pointer drag → yaw/pitch with inertia. Returns a state object read each frame.
export function orbitControl(el, { yaw = 0, pitch = 0, autoSpin = 0.05 } = {}) {
  const s = { yaw, pitch, vy: 0, vp: 0, dragging: false, idle: 10 };
  let lx = 0, ly = 0;
  el.style.touchAction = 'pan-y';
  el.addEventListener('pointerdown', (e) => {
    s.dragging = true; lx = e.clientX; ly = e.clientY;
    el.setPointerCapture(e.pointerId);
  });
  el.addEventListener('pointermove', (e) => {
    if (!s.dragging) return;
    const k = 4 / Math.max(el.clientWidth, 1);
    const dx = (e.clientX - lx) * k, dy = (e.clientY - ly) * k;
    lx = e.clientX; ly = e.clientY;
    s.yaw -= dx; s.pitch -= dy;
    s.vy = -dx * 60; s.vp = -dy * 60;
    s.idle = 0;
  });
  const end = () => { s.dragging = false; };
  el.addEventListener('pointerup', end);
  el.addEventListener('pointercancel', end);

  s.update = (dt) => {
    if (!s.dragging) {
      s.idle += dt;
      s.yaw += s.vy * dt; s.pitch += s.vp * dt;
      const damp = Math.exp(-dt * 3);
      s.vy *= damp; s.vp *= damp;
      if (s.idle > 2.5) s.yaw += autoSpin * dt;
    }
    s.pitch = Math.max(-1.45, Math.min(1.45, s.pitch));
  };
  return s;
}
