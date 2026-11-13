# PulseGrid source record

The user supplied `pulsegrid_by_purpletwilight21858029 (3).zip` on 12 November 2026. The filename attributes the work to **PurpleTwilight**. Its SHA-256 is:

`68b589b0fff0cb302bf274cc56713e96162397b278eadd8a8ac6814ff9d6869b`

The archive was inspected as reference material and was not executed. It contains a browser presentation with three render modes, SID-oriented audio code and assets, C64 font/data files, GLSL shaders, JavaScript camera/rendering code, and an embedded native-source release archive. Its own audit describes a 25 Hz visual cadence, 138 BPM fallback timing, a rotating rounded cube, nested source-table layers, orbital geometry, a floor grid, beat envelopes and a world-space voxel scroller.

Scene 73 is an original native C++ 3D model evolved from that visual composition. `PulseGridGeometry.cpp` independently constructs the PulseGrid Reactor: a rounded structural chassis, a faceted counter-rotating pulse core, six coiled power buses and three articulated energy gimbals, with expanding floor rings, a receding grid and a geometric heartbeat rail as its environment. It uses the supplied cadence as a creative reference, while the engine supplies its own music clock, HDR wires, bloom, gloss, backgrounds and camera pipeline. No JavaScript, GLSL, SID binary, font, music or embedded archive from the supplied package is copied into this repository.

Scene 74 is a separate C64-native reinterpretation rather than a filter over scene 73. Its geometry is rebuilt from block primitives, a voxel core, octagonal coils and extruded 5×7 C64 lettering at a stepped 50 Hz PAL cadence. The post pass places a hard-sampled 320×200 active screen inside a 384×272 cropped PAL display raster, applies 4×4 ordered dithering and maps logical pixels exclusively to the fixed 16-colour VIC-II palette. A later display stage adds the default blue/light-blue screen and border, beam-shaped scanlines, alternating-line PAL chroma phase and subtle phosphor persistence. It emulates the visual constraints in the native 3D engine; it is not an executable for original C64 hardware or a cycle-accurate VIC-II emulator.

The supplied archive contains no explicit license covering the effect as a whole. This provenance record does not grant or imply additional rights. Obtain permission from the relevant rights holder for uses that require it.
