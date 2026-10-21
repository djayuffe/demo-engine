import { createChamber } from './chamber.js';
import { createGrin, SOURCE } from './grin.js';

const slider = document.querySelector('#curvature');
const value = document.querySelector('#range-value');
const readout = document.querySelector('#curvature-readout');
const stage = document.querySelector('.ray-stage');

function gpu(el, init) {
  try {
    const api = init();
    el.parentElement.classList.add('gpu-on');
    return api;
  } catch (err) {
    console.warn(`GPU figure #${el.id} fell back: ${err && err.message}`);
    el.remove();
    return null;
  }
}

gpu(document.querySelector('#chamber-object'), () => createChamber(document.querySelector('#chamber-object'), 'object'));
gpu(document.querySelector('#chamber-cube'), () => createChamber(document.querySelector('#chamber-cube'), 'cube'));
gpu(document.querySelector('#chamber-flat'), () => createChamber(document.querySelector('#chamber-flat'), 'flat'));
gpu(document.querySelector('#chamber-hyper'), () => createChamber(document.querySelector('#chamber-hyper'), 'hyper'));
const grin = gpu(document.querySelector('#grin'), () => createGrin(document.querySelector('#grin')));

if (grin) {
  const pin = stage.querySelector('.point-in');
  pin.style.left = `calc(${SOURCE.x * 100}% - 2.5px)`;
  pin.style.bottom = `calc(${SOURCE.y * 100}% - 5px)`;
  pin.style.flexDirection = 'row-reverse';
  pin.querySelector('span').textContent = 'SOURCE';
  stage.querySelector('.point-out span').textContent = 'n RISES TOWARD THE FLOOR ↓';
}

// SVG fallback path (only used when WebGL2 is unavailable).
const ray = document.querySelector('.main-ray');
const faintRay = document.querySelector('.ray-trace.faint');
const pulse = document.querySelector('.ray-pulse');

function updateCurvature() {
  const amount = Number(slider.value);
  value.textContent = amount;
  readout.textContent = `INDEX GRADIENT  /  ${amount}%`;
  slider.style.background = `linear-gradient(90deg, var(--mint) 0%, #a28bff ${amount}%, #343a45 ${amount}%)`;
  if (grin) {
    grin.setCurvature(amount / 100);
    return;
  }
  const bend = 25 + (amount / 100) * 90;
  const path = `M36 230 C${180 - bend} 220 ${160 + bend} ${65 + bend * .3} 360 78 S${520 - bend * .35} ${242 - bend} 678 205 818 ${54 + bend * .25} 975 75`;
  ray.setAttribute('d', path);
  faintRay.setAttribute('d', path);
  pulse.style.offsetPath = `path('${path}')`;
}
slider.addEventListener('input', updateCurvature);

function bindSeg(el, onChange) {
  el.addEventListener('click', (e) => {
    const b = e.target.closest('button');
    if (!b) return;
    el.querySelectorAll('button').forEach((x) => x.setAttribute('aria-pressed', String(x === b)));
    onChange(b.dataset.v);
  });
}
const spectrum = document.querySelector('#spectrum');
if (grin) bindSeg(spectrum, (v) => grin.setWhite(v === 'white'));
else spectrum.remove();
updateCurvature();

const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
if (!grin && !reduceMotion && pulse.animate) {
  pulse.animate([{ offsetDistance: '0%' }, { offsetDistance: '100%' }], {
    duration: 5200,
    iterations: Infinity,
    easing: 'linear'
  });
}

// ---------------------------------------------------------------- instrument
import { createInstrument, PHYS } from './instrument.js';

const benchCanvas = document.querySelector('#bench');
const bench = gpu(benchCanvas, () => createInstrument(benchCanvas));
document.querySelector('#n-corner').textContent = PHYS.nCorner.toFixed(2);
const questions = document.querySelectorAll('.question[data-preset]');
if (bench) {
  const keys = { 'b-medium': 'medium', 'b-steps': 'steps', 'b-curved': 'curved', 'b-hole': 'hole', 'b-white': 'white', 'b-nout': 'nout', 'b-inside': 'inside' };
  const DEFAULTS = { medium: 1, steps: 0, curved: 1, hole: 0, white: 0, nout: 0, inside: 0 };
  // Each open question, staged on the bench: what the failure mode looks like.
  const PRESETS = {
    blend:   { medium: 0 },   // no index blend: an ordinary glass block
    smooth:  { steps: 1 },    // gradient printed in discrete layers
    color:   { white: 1 },    // broad-spectrum LEDs disperse
    corners: { curved: 0 },   // straight-sided faces: seams at the vertices
  };
  const state = { ...DEFAULTS };
  const readoutEl = document.querySelector('#bench-readout');

  const apply = (key, v) => {
    state[key] = v;
    bench.set(key, key === 'nout' ? (v ? 1.333 : 1.0) : v);
  };
  const showSeg = (id, v) => document.getElementById(id).querySelectorAll('button')
    .forEach((b) => b.setAttribute('aria-pressed', String(Number(b.dataset.v) === v)));

  const describe = () => {
    let n = `n ${PHYS.nCentre.toFixed(2)}`;
    if (state.medium) n += state.steps ? ` → ${PHYS.nCorner.toFixed(2)} IN ${PHYS.layers} LAYERS` : ` → ${PHYS.nCorner.toFixed(2)}`;
    const samples = bench.converged ? 'CONVERGED' : `${String(bench.frames).padStart(3, '0')} / ${bench.maxFrames}`;
    return `${n}  /  ${state.nout ? 'WATER 1.33' : 'AIR 1.00'}  /  ${samples}`;
  };
  for (const [id, key] of Object.entries(keys)) {
    bindSeg(document.getElementById(id), (v) => { apply(key, Number(v)); markQuestion(); });
  }

  const markQuestion = () => questions.forEach((q) => {
    const p = { ...DEFAULTS, ...PRESETS[q.dataset.preset] };
    q.classList.toggle('active', Object.keys(p).every((k) => p[k] === state[k]));
  });
  questions.forEach((q) => q.addEventListener('click', () => {
    const target = { ...DEFAULTS, ...PRESETS[q.dataset.preset] };
    for (const [id, key] of Object.entries(keys)) {
      if (state[key] !== target[key]) apply(key, target[key]);
      showSeg(id, target[key]);
    }
    markQuestion();
  }));

  let shown = '';
  (function tickReadout() {
    const text = describe();
    if (text !== shown) { readoutEl.textContent = text; shown = text; }
    setTimeout(tickReadout, 200);
  })();
} else {
  questions.forEach((q) => { q.removeAttribute('href'); q.querySelector('b').remove(); });
}
