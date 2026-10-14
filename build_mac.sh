#!/usr/bin/env bash
# One-command macOS build: installs missing Homebrew deps, configures a Release build, builds, runs the
# tests and (with --run) starts the demo.   Usage: ./build_mac.sh [--run] [--clean] [-- demo args...]
set -euo pipefail
cd "$(dirname "$0")"
RUN=0; CLEAN=0
while [[ $# -gt 0 ]]; do case "$1" in --run) RUN=1;; --clean) CLEAN=1;; --) shift; break;; *) echo "unknown option $1"; exit 2;; esac; shift; done
[[ "$(uname -s)" == Darwin ]] || { echo "build_mac.sh is for macOS"; exit 2; }
command -v brew >/dev/null || { echo "Homebrew is required: https://brew.sh"; exit 2; }
for f in cmake glfw sdl2 libopenmpt pkg-config; do brew list --versions "$f" >/dev/null 2>&1 || brew install "$f"; done
xcode-select -p >/dev/null 2>&1 || { echo "Install the Xcode command line tools: xcode-select --install"; exit 2; }
[[ $CLEAN == 1 ]] && rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build -j"$(sysctl -n hw.ncpu)"
(cd build && ./geometry_tests)
echo "Built: $(pwd)/build/impossible_wireframe ($(uname -m), macOS $(sw_vers -productVersion))"
[[ $RUN == 1 ]] && exec ./build/impossible_wireframe "$@"
exit 0
