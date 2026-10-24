# Convenience wrapper around the CMake build. The real build system is
# CMakeLists.txt; this Makefile gives you `make`-style targets with a few
# quality-of-life additions (auto-reconfigure when the cache goes stale,
# parallel builds, and a `diag` target that dumps the resolved toolchain).
#
# Usage:
#   make            # configure + build (Release)
#   make test       # build + run ctest
#   make run        # build + launch Hold Infinity (tempo from the soundtrack)
#   make run-atlas  # build + launch the first live GPU Atlas scene
#   make run-aviation # launch the supplied parametric 3D point effect
#   make run-silicon# the wireframe demo with Silicon Dancer by Drozerix
#   make run-hello  # build + launch the minimal hello demo
#   make run-ripple # build + launch the GPU-tessellated ripple demo
#   make run-pulse  # launch with the CC0 Wireframe Pulse track
#   make clean      # remove the build/ directory
#   make reconfigure# force a fresh CMake configure (new build dir)
#   make diag       # dump the resolved compiler / SDK / GLFW / SDL2 / openmpt
#   make help       # show this list
#
# Extra CMake flags can be passed as CMAKE_EXTRA, e.g.:
#   make CMAKE_EXTRA=-DIW_WARNINGS_AS_ERRORS=OFF
#   make reconfigure CMAKE_EXTRA=-DIW_SYSROOT=$(xcrun --show-sdk-path)
#
# To build audio in, the toolchain must expose SDL2 + libopenmpt via
# CMake/PkgConfig (brew install sdl2 libopenmpt on macOS;
# apt install libsdl2-dev libopenmpt-dev on Linux; the CI does this).

BUILD_DIR   ?= build
TYPE        ?= Release
NPROC       ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CMAKE       ?= cmake
CTEST       ?= ctest
BIN         := $(BUILD_DIR)/hold_infinity
WIREFRAME   := $(BUILD_DIR)/impossible_wireframe
HELLO       := $(BUILD_DIR)/hello_demo
RIPPLE      := $(BUILD_DIR)/ripple_demo
ATLAS_RENDER := $(BUILD_DIR)/demos/atlas/atlas_render
ATLAS_ADVANCED := $(BUILD_DIR)/demos/atlas/atlas_advanced

# Common CMake flags. -DIW_BUILD_TESTS=ON is explicit so a user who disables
# tests has to do so deliberately. CMAKE_EXTRA is appended last so it can
# override anything above.
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=$(TYPE) -DIW_BUILD_TESTS=ON $(CMAKE_EXTRA)

# Fingerprint of the inputs that should trigger a reconfigure: the generator,
# CMakeLists.txt, and the build type. If any changes, the cache is stale and
# we reconfigure automatically. (A full hash of every source would be overkill;
# CMake itself re-runs when CMakeLists.txt changes — this is a backstop for
# the build-type / generator case.)
STAMP := $(BUILD_DIR)/.stamp

.PHONY: all configure build test run run-atlas run-aviation run-atlas-reference run-atlas-advanced run-wireframe run-silicon run-hello run-ripple run-pulse clean reconfigure diag help stamp

all: build

stamp:
	@mkdir -p $(BUILD_DIR)
	@# Reconfigure if the build dir is missing, the generator changed, the
	@# CMakeLists/Makefile/build-type stamp is older than the sources, OR the
	@# cached CMAKE_BUILD_TYPE differs from the requested TYPE. The last check
	@# is the one mtime alone can't catch: `make` then `make TYPE=Debug` would
	@# otherwise silently reuse the stale Release cache (the stamp is not older
	@# than CMakeLists.txt, so no reconfigure fires). We compare the *value*
	@# actually in CMakeCache.txt against the requested type.
	@cached=; if [ -f $(BUILD_DIR)/CMakeCache.txt ]; then \
	  cached=$$(grep -E '^CMAKE_BUILD_TYPE:' $(BUILD_DIR)/CMakeCache.txt 2>/dev/null | head -1 | cut -d= -f2); fi; \
	if [ ! -d $(BUILD_DIR) ] || \
	    [ ! -f $(BUILD_DIR)/CMakeCache.txt ] || \
	    [ ! -f $(STAMP) ] || \
	    [ "$$cached" != "$(TYPE)" ] || \
	    [ $(STAMP) -ot CMakeLists.txt ] || \
	    [ $(STAMP) -ot Makefile ]; then \
	  if [ -n "$$cached" ] && [ "$$cached" != "$(TYPE)" ]; then \
	    echo "build type changed $$cached -> $(TYPE), reconfiguring"; \
	  fi; \
	  rm -rf $(BUILD_DIR); \
	  $(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_FLAGS); \
	  date > $(STAMP); \
	else \
	  echo "build/ up to date — reusing existing cache ($(TYPE))"; \
	fi

configure:
	rm -rf $(BUILD_DIR)
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)
	@date > $(STAMP)

build: stamp
	$(CMAKE) --build $(BUILD_DIR) --parallel $(NPROC)

test: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure

run: build
	$(BIN)

run-atlas: build
	$(BIN) --infinity-scene 16

run-aviation: build
	$(BIN) --infinity-scene 26

run-atlas-reference: build
	$(ATLAS_RENDER) $(BUILD_DIR)/atlas-reference.ppm 0.9 480
	@echo "Wrote $(BUILD_DIR)/atlas-reference.ppm"

run-atlas-advanced: build
	$(ATLAS_ADVANCED) $(BUILD_DIR)/atlas-advanced.ppm 640 0.8 0
	@echo "Wrote $(BUILD_DIR)/atlas-advanced.ppm"

run-wireframe: build
	$(WIREFRAME)

run-silicon: build
	$(BIN) --music demos/wireframe/assets/music/drozerix_-_silicon_dancer.mod

run-hello: build
	$(HELLO)

run-ripple: build
	$(RIPPLE)

run-pulse: build
	$(BIN) --bpm 128 --music demos/wireframe/assets/music/wireframe_pulse.wav

clean:
	rm -rf $(BUILD_DIR)

reconfigure:
	rm -rf $(BUILD_DIR)
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)
	@date > $(STAMP)

# Dump the resolved toolchain: compiler, SDK (macOS), and each detected
# dependency target. Useful for diagnosing "why didn't audio get built in?".
diag: stamp
	@echo "--- CMake version ---"
	@$(CMAKE) --version | head -1
	@echo "--- Build type ---"
	@grep -E 'CMAKE_BUILD_TYPE' $(BUILD_DIR)/CMakeCache.txt || echo "(default)"
	@echo "--- macOS SDK (if applicable) ---"
	@grep -E 'CMAKE_OSX_SYSROOT|CMAKE_OSX_ARCHITECTURES' $(BUILD_DIR)/CMakeCache.txt || echo "(not macOS)"
	@echo "--- GLFW target ---"
	@grep -E 'GLFW3_DIR|GlfwFallback|GLFW_INCLUDE_DIR|GLFW_LIBRARY' $(BUILD_DIR)/CMakeCache.txt || echo "(none found)"
	@echo "--- SDL2 target ---"
	@grep -E 'SDL2_DIR|SDL2_INCLUDE_DIR|SDL2_LIBRARY|PkgConfig' $(BUILD_DIR)/CMakeCache.txt | head -5 || echo "(none found)"
	@echo "--- libopenmpt target ---"
	@grep -E 'OPENMPT_INCLUDE_DIR|OPENMPT_LIBRARY|libopenmpt' $(BUILD_DIR)/CMakeCache.txt || echo "(none found)"
	@echo "--- OpenGL target (macOS) ---"
	@grep -E 'IW_OPENGL_LIB|IW_OPENGL_INCLUDE_DIR|OPENGL_INCLUDE_DIR' $(BUILD_DIR)/CMakeCache.txt || echo "(none found)"

help:
	@echo "Targets:"
	@echo "  all / build     Configure (if needed) + build (Release, parallel)"
	@echo "  test            Build + run ctest"
	@echo "  run             Build + launch Hold Infinity (music tempo)"
	@echo "  run-atlas       Build + launch the first live GPU Atlas scene"
	@echo "  run-aviation    Build + launch the supplied parametric 3D point effect"
	@echo "  run-atlas-reference Generate the four-panel CPU Atlas reference image"
	@echo "  run-atlas-advanced  Generate the advanced CPU Atlas reference image"
	@echo "  run-wireframe   Build + launch the previous wireframe show"
	@echo "  run-hello       Build + launch the minimal hello demo"
	@echo "  run-ripple      Build + launch the GPU-tessellated ripple demo"
	@echo "  run-pulse       Build + launch Hold Infinity with the CC0 Pulse track"
	@echo "  configure       Wipe build/ and reconfigure"
	@echo "  reconfigure     Alias for configure"
	@echo "  diag            Dump the resolved toolchain (SDK, GLFW, SDL2, openmpt)"
	@echo "  clean           Remove build/"
	@echo "  help            This message"
	@echo ""
	@echo "Vars:"
	@echo "  BUILD_DIR=build  Output directory"
	@echo "  TYPE=Release     CMake build type"
	@echo "  NPROC=$(NPROC)          Parallelism"
	@echo "  CMAKE=cmake        CMake binary"
	@echo "  CTEST=ctest        ctest binary"
	@echo "  CMAKE_EXTRA=       Extra CMake flags (e.g. -DIW_WARNINGS_AS_ERRORS=ON,"
	@echo "                     -DIW_SYSROOT=\$$(xcrun --show-sdk-path))"
