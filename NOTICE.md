# Notice

Copyright (c) 2026 Ulf Bertilsson.

This repository contains a generic wireframe demo engine, three demos built on it
(including the Impossible Wireframe demo), geometry generators, validation tests,
shader code, documentation and build scripts.
Third-party dependencies remain under their own licenses.

Bundled music:

- **"Stars (4ch. version)" by cs127** (`demos/wireframe/assets/music/cs127_-_stars_4ch.mod`),
  the wireframe demo's default soundtrack. Source: The Mod Archive, module 201917
  (https://modarchive.org/index.php?request=view_by_moduleid&query=201917).
  Licensed under Creative Commons Attribution 4.0
  (https://creativecommons.org/licenses/by/4.0/). Redistributed unmodified; the
  demo shows this credit in its ticker while the track plays.

- **"Digital Rendezvous" by Drozerix** (`demos/wireframe/assets/music/drozerix_-_digital_rendezvous.xm`),
  second in the wireframe demo's playlist. Source: The Mod Archive, module 180821
  (https://modarchive.org/index.php?request=view_by_moduleid&query=180821). Public Domain.
  Redistributed unmodified; credited in the ticker while it plays.

Other music:

- Drozerix — Silicon Dancer (`drozerix_-_silicon_dancer.mod`), listed by the
  Quinlight Audio project as Public Domain.

The bundled tracker module and the generated `wireframe_pulse.wav` are committed under `demos/wireframe/assets/music/` so the demo plays out of the box; `demos/wireframe/assets/music/README.md` lists their sources and licences. The UBER logo cards in `demos/wireframe/assets/logos/` are the author's own artwork.

Third-party code vendored in `thirdparty/`: `stb_image.h` and `stb_image_write.h` (Sean Barrett, public domain / MIT).

Imported Hold Infinity optical renderer sources are credited separately in `demos/infinity/SOURCE.md`; original files and source hashes are preserved under that directory. They are not relicensed by the engine's MIT license.

The user-supplied DMT Atlas v0.3 reference sources and their derivatives are recorded in `demos/atlas/SOURCE.md`. Original files and SHA-256 hashes are preserved. The archive provides no separate license; inclusion does not relicense it under the engine MIT license.


## Additional CC0 music (2026-10-24)

The bundled Melodic EDM (Fupi), Black Diamond (Joth), Moil (Ruskerdax), and Synthwave
15k (The Cynic Project / cynicmusic) are distributed unmodified under their source
pages' CC0 designation. Their rights are separate from the engine source code.
See [music credits](demos/wireframe/assets/music/README.md) and
[the pinned source manifest](demos/wireframe/assets/music/additional_music.json)
for source/download URLs, licensing links, author credits and SHA-256 checksums.
The Cynic Project: https://cynicmusic.com/ and https://pixelsphere.org/.
