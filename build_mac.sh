#!/usr/bin/env bash
# build_mac.sh: one-command macOS build. Checks the toolchain, installs missing Homebrew packages in
# one batch, configures with Ninja (and ccache when present), builds, runs the tests in parallel and
# optionally launches a demo. Written for the macOS system bash (3.2): no bash-4 features.
set -euo pipefail
cd "$(dirname "$0")"

usage() {
  cat <<'EOF'
Usage: ./build_mac.sh [options] [-- demo arguments]

  -r, --run             launch the demo after a successful build
  -d, --demo NAME       demo to build/run: infinity (default), wireframe, hello, ripple
  -c, --clean           delete the build directory first
      --debug           Debug build (build-debug/)
      --profile         optimised build with symbols, RelWithDebInfo (build-profile/)
      --lto             link-time optimisation (Release only, slower link)
      --no-tests        skip the tests and build only the selected demo (fastest)
      --no-deps         do not check or install Homebrew packages
  -j, --jobs N          parallel jobs (default: all cores)
  -v, --verbose         show the full configure output and compiler command lines
  -h, --help            this help

Examples:
  ./build_mac.sh --run
  ./build_mac.sh --run --demo wireframe -- --no-logo --background-style auto
  ./build_mac.sh --no-tests --lto --run
EOF
}

die() { printf 'build_mac.sh: %s\n' "$*" >&2; exit 2; }
say() { printf '\033[1m==>\033[0m %s\n' "$*"; }

RUN=0; CLEAN=0; TESTS=1; DEPS=1; LTO=0; VERBOSE=0; TYPE=Release; DEMO=infinity; JOBS=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    -r|--run) RUN=1 ;;
    -c|--clean) CLEAN=1 ;;
    -d|--demo) [[ $# -ge 2 ]] || die "--demo needs a name"; DEMO="$2"; shift ;;
    --demo=*) DEMO="${1#*=}" ;;
    --debug) TYPE=Debug ;;
    --profile) TYPE=RelWithDebInfo ;;
    --lto) LTO=1 ;;
    --no-tests) TESTS=0 ;;
    --no-deps) DEPS=0 ;;
    -j|--jobs) [[ $# -ge 2 ]] || die "--jobs needs a number"; JOBS="$2"; shift ;;
    -j*) JOBS="${1#-j}" ;;
    -v|--verbose) VERBOSE=1 ;;
    -h|--help) usage; exit 0 ;;
    --) shift; break ;;
    *) usage >&2; die "unknown option: $1" ;;
  esac
  shift
done

case "$DEMO" in
  infinity|hold_infinity) BIN=hold_infinity ;;
  wireframe|impossible_wireframe) BIN=impossible_wireframe ;;
  hello|hello_demo) BIN=hello_demo ;;
  ripple|ripple_demo) BIN=ripple_demo ;;
  *) die "unknown demo '$DEMO' (infinity, wireframe, hello, ripple)" ;;
esac
[[ -z "$JOBS" ]] && JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 8)"
[[ "$JOBS" =~ ^[1-9][0-9]*$ ]] || die "--jobs must be a positive number"
[[ $LTO == 1 && $TYPE != Release ]] && die "--lto only applies to Release builds"

# ---- toolchain checks (before installing anything) ----------------------------------------------
[[ "$(uname -s)" == Darwin ]] || die "this script is for macOS; on Linux use 'make' (see README)"
xcode-select -p >/dev/null 2>&1 || die "install the Xcode command line tools first: xcode-select --install"
if [[ "$(sysctl -n sysctl.proc_translated 2>/dev/null || echo 0)" == 1 ]]; then
  say "warning: this shell runs under Rosetta; the build will be x86_64. Open a native arm64 terminal for full speed."
fi

# ---- Homebrew packages: one listing, one batched install ---------------------------------------
if [[ $DEPS == 1 ]]; then
  command -v brew >/dev/null || die "Homebrew is required: https://brew.sh (or pass --no-deps)"
  # cmake/ninja/pkg-config/glfw build the engine; sdl2 + libopenmpt give music, libsndfile the
  # MP3/FLAC/OGG tracks. ccache is used when present but never installed.
  WANT="cmake ninja pkg-config glfw sdl2 libopenmpt libsndfile"
  HAVE=" $(brew list --formula -1 2>/dev/null | tr '\n' ' ') "
  MISSING=""
  for f in $WANT; do [[ "$HAVE" == *" $f "* ]] || MISSING="$MISSING $f"; done
  if [[ -n "$MISSING" ]]; then
    say "installing:$MISSING"
    # shellcheck disable=SC2086
    HOMEBREW_NO_INSTALL_CLEANUP=1 HOMEBREW_NO_ENV_HINTS=1 brew install $MISSING
  fi
fi
command -v cmake >/dev/null || die "cmake not found (brew install cmake)"

# ---- configure --------------------------------------------------------------------------------
case "$TYPE" in
  Release) BUILD=build ;;
  Debug) BUILD=build-debug ;;
  *) BUILD=build-profile ;;
esac
GEN="Unix Makefiles"; command -v ninja >/dev/null && GEN=Ninja
# A build directory made with another generator cannot be reused; start that one fresh.
if [[ -f "$BUILD/CMakeCache.txt" ]] && ! grep -qx "CMAKE_GENERATOR:INTERNAL=$GEN" "$BUILD/CMakeCache.txt"; then
  say "generator changed to $GEN: recreating $BUILD/"
  CLEAN=1
fi
[[ $CLEAN == 1 ]] && rm -rf "$BUILD"

ARGS=(-S . -B "$BUILD" -G "$GEN" "-DCMAKE_BUILD_TYPE=$TYPE" "-DIW_ENABLE_LTO=$([[ $LTO == 1 ]] && echo ON || echo OFF)"
      "-DIW_BUILD_TESTS=$([[ $TESTS == 1 ]] && echo ON || echo OFF)")
command -v ccache >/dev/null && ARGS+=(-DCMAKE_CXX_COMPILER_LAUNCHER=ccache)

START=$SECONDS
say "configure $TYPE ($GEN, $JOBS jobs$([[ $LTO == 1 ]] && echo ', LTO')$(command -v ccache >/dev/null && echo ', ccache'))"
mkdir -p "$BUILD"
if [[ $VERBOSE == 1 ]]; then
  cmake "${ARGS[@]}"
elif ! cmake "${ARGS[@]}" >"$BUILD/configure.log" 2>&1; then
  cat "$BUILD/configure.log" >&2
  die "configure failed (log: $BUILD/configure.log)"
else
  # Surface what the user would otherwise miss: missing optional audio and any warnings.
  grep -E "libsndfile|Link-time|Pinned macOS SDK" "$BUILD/configure.log" | sed 's/^-- /   /' || true
  grep -A3 "CMake Warning" "$BUILD/configure.log" >&2 || true
fi

# ---- build + test -----------------------------------------------------------------------------
BUILD_ARGS=(--build "$BUILD" --parallel "$JOBS")
[[ $TESTS == 0 ]] && BUILD_ARGS+=(--target "$BIN")
[[ $VERBOSE == 1 ]] && BUILD_ARGS+=(--verbose)
say "build $([[ $TESTS == 0 ]] && echo "$BIN" || echo "all targets")"
cmake "${BUILD_ARGS[@]}"

if [[ $TESTS == 1 ]]; then
  say "test"
  ctest --test-dir "$BUILD" --output-on-failure --parallel "$JOBS"
fi

[[ -x "$BUILD/$BIN" ]] || die "expected binary missing: $BUILD/$BIN"
say "built $BUILD/$BIN in $((SECONDS - START)) s ($(uname -m), macOS $(sw_vers -productVersion 2>/dev/null || echo ?))"
if [[ $RUN == 1 ]]; then
  exec "./$BUILD/$BIN" "$@"
fi
