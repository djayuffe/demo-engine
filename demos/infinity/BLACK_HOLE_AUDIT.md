# Fractal black-hole renderer audit — 2026-10-25

Scope: native scene 28, including camera rays, curvature integration, event-horizon termination, accretion-disk intersection and volume, photon rings, GPU particles, jets, star background, HDR output and adaptive GPU timing. This is an artistic weak-field ray model rather than a full Schwarzschild/Kerr geodesic solver.

## Corrected findings

| Area | Finding | Correction |
|---|---|---|
| Curvature | Direction deflection was applied once per loop without the adaptive spatial step, so smaller near-horizon steps changed the effective gravity | Scale curvature by travelled distance; retain bounded normalization and horizon denominators |
| Disk surface | A plane crossing shaded at the end of the ray step, producing visible stepped bands at grazing angles | Interpolate the exact segment/plane crossing before evaluating radius, orbit velocity and material |
| Camera parallax | The world-space eye offset was multiplied by the camera matrix a second time | Add eye translation directly to the world-space camera origin |
| Star field | A random value illuminated an entire spherical grid cell | Place two filtered, jittered subpixel star layers within their cells, with independent color temperature and brightness |
| Disk depth | Only the infinitesimal plane surface contributed plasma | Add a bounded thin-volume density with radial falloff, vertical scale height, spiral filaments and exponential absorption |
| Ring detail | Photon rings were uniform around the full circumference | Add restrained rotating caustic modulation and paired blue lens arcs |
| Sparse foreground motion | The ray pass described continuous plasma but lacked individually readable flying matter | Add 98,304 accretion parcels with Kepler-scaled orbit speed, radial infall, scale height and Doppler color, plus 32,768 accelerating bipolar jet parcels in widening magnetic helices |
| Particle/horizon ordering | Additive point sprites have no depth surface to reject far-side matter | Analytically occult particles behind the event-horizon silhouette in camera space |

The 96-step loop retains adaptive spatial steps, a hard event-horizon termination, bounded curvature, finite direction fallbacks, finite disk radii and an outer escape bound. Doppler beaming uses the local orbital tangent and line-of-sight direction. The 131,072-particle overlay is generated from `gl_VertexID`; it has no geometry buffer, CPU simulation or per-frame upload. Bass changes gravity, orbital speed and disk energy, mid energy changes spiral flow, treble excites stars and jet knots, and beat attacks expand and illuminate the photon ring and particles.

## Validation

- Strict-warning Release build and all nine test targets pass.
- Shader contract tests retain step-scaled curvature, world-space parallax, segment-interpolated disk crossings, filtered stars, volumetric density, zero-upload particle generation, Kepler motion, radial infall, ballistic jets, magnetic helices and horizon occultation.
- Finite RGBA16F readback passes at the 1,998,100-pixel Metal ceiling.
- Fixed left/right camera captures differ across the full 1280 × 720 frame, with mean absolute RGB differences of 13.081, 3.745 and 3.919 display-code values.
- Matched 120-frame M1 Pro profiling at 640 × 360 logical / 1280 × 720 framebuffer improved from 19.193 ms average and 31.572 ms p95 to 16.649 ms average and 20.652 ms p95. Both runs used adaptive 60 FPS control, six-frame startup exclusion, silent 132 BPM, fixed step, centered eye and no vsync.
- With all 131,072 physics particles enabled, the same profile measures 17.186 ms average and 20.014 ms p95. The zero-upload overlay adds 0.537 ms to the average while retaining the adaptive renderer's 0.64 terminal quality fraction.
- The adaptive governor reduced the learned pixel fraction from 1.0 to 0.8 and then 0.64 when the high-resolution workload exceeded the 16.67 ms target. It does not reduce ray steps or material features.

Reproduce the focused check:

```sh
./build/hold_infinity --infinity-scene 28 --window 640x360 --frames 120 \
  --fixed-step 0.0166667 --no-ticker --silent --bpm 132 --no-vsync \
  --infinity-eye 0 0 --gpu-profile --validate-output
```

The full native suite remains `python3 tools/validate_infinity.py --binary build/hold_infinity`.
