# Atlas integration validation — 22 October 2026

Native platform: Apple M1 Pro, OpenGL 4.1 Metal 91.7. Strict-warning Release build (`IW_WARNINGS_AS_ERRORS=ON`). Other GPU drivers and physical Windows/Linux machines were not exercised in this pass.

- All **7 CTest checks** pass, including both original geometry suites, both CPU render smoke checks and the new integration suite.
- New geometry checks cover **250 rotating sections**, cube/octahedron/coplanar/empty/tangent/zero-volume cases, convex bounds, Euler topology, general Lorentz camera invariants, hierarchy containment and **12,000 BVH versus brute-force rays**.
- The Atlas integration suite passes AddressSanitizer and UndefinedBehaviorSanitizer.
- All **26 native scenes** render successfully. The complete 398-frame accelerated automatic run crosses every scene and returns to scene zero at beat 833.8.
- Full float ray-state comparisons pass for both ray-lab modes, three curvatures and landscape/portrait orientations; optical masks 31, 95 and 127, GPU target lifetime checks and 400-sample still convergence pass.
- Atlas checks cover ten animated modes, five portrait/displaced-eye cases, five byte-identical short/long pause pairs, reduced motion, zero/capped exposure and invalid numeric arguments.
- Raw and filtered Atlas HDR targets remain finite, including first-frame history initialization and all scene boundaries.
- All original archive file hashes match `source.json`.

The generic reference shader and each specialized Atlas mode were captured with identical deterministic inputs. The lowest image match was **66.12 dB PSNR / 0.99996 SSIM** (face emergence); other matches were at least 90.93 dB or pixel identical. The four modes touched by the final bounds/glyph refinement were compared again: H³ 92.77 dB, portal 101.41 dB, event graph and three-view mode pixel identical. Every result exceeds the existing 40 dB / 0.98 criteria.

[Recorded GPU measurements](../../docs/atlas-gpu-results.json) include reference and specialized timings. These are one 60-frame run per path with 50 timed samples, at a 120,000-pixel ceiling. For example, the final H³ sphere run measured 1.322 ms reference versus 0.599 ms accelerated; the portal measured 0.623 versus 0.539 ms. Short-run variance prevents treating these as universal speedup figures. The generic and accelerated modes remain available for repeat measurements on other hardware.

[Gallery](../../docs/atlas-contact.jpg) uses actual native captures. [Feature coverage and research boundaries](README.md) separate the implemented archive algorithms from open roadmap topics.

## 4.1 integration check — 24 October 2026

After pulling `origin/main`, the strict-warning build and all seven CTest checks pass. All 26 native scenes, full optical ray-state comparisons, target lifetime checks, still convergence and the complete automatic cycle pass with the shared highlight pyramid enabled.

Reference-versus-specialized image checks for scenes 14, 18, 20, 23, 24 and 25 pass: the lowest finite PSNR is 90.29 dB and SSIM is 1.0 throughout. [Recorded measurements](../../docs/effects-gpu-results.json) include the new H³ shadow rays and HDR passes. Single-run GPU timing varies; some specialized paths were slower in this sample, so these results do not establish a uniform speedup.

The extended control checks cover pause identity, portrait views, displaced eyes, fixed orbits, grazing-angle relief, reduced motion, exposure and original source hashes. New CPU invariants cover audio update subdivision, release envelopes, invalid inputs, pause/resume, backward seeks, reduced-motion rate, periodic breathing, breathing slice topology and padded animated sphere bounds.
