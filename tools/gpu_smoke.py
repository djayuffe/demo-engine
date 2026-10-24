#!/usr/bin/env python3
"""Exercise real GL shader compilation and finite HDR output; requires a desktop session."""
import argparse
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=Path("build"))
    options = parser.parse_args()
    suffix = ".exe" if sys.platform == "win32" else ""

    def run(name, extra=(), expected=None):
        binary = (options.build / (name + suffix)).resolve()
        args = [str(binary), "--frames", "4", "--fixed-step", "0.016667",
                "--window", "640x400", "--no-ticker", *extra]
        result = subprocess.run(args, capture_output=True, text=True, timeout=60)
        log = result.stdout + result.stderr
        if (result.returncode or "--frames 4 reached" not in log
                or any(word in log.lower() for word in ("unavailable", "fallback (log once)", "sw vertex", "software vertex", "opengl error"))
                or (expected and expected not in log)):
            raise RuntimeError(f"{' '.join(args)}\n{log}")
        return log

    for demo in ("hello_demo", "ripple_demo", "impossible_wireframe"):
        log = run(demo, ("--no-post", "--stats"))
        if demo in ("hello_demo", "ripple_demo") and "Mesh uploads: vertices=1 indices=1" not in log:
            raise RuntimeError("Static geometry was not uploaded exactly once: " + log)
        run(demo)
        print(f"{demo}: post and bypass paths passed", flush=True)
    run("hello_demo", ("--window", "320x200", "--help-overlay"))
    run("impossible_wireframe", ("--help-overlay",))
    run("hold_infinity", ("--hud", "--infinity-scene", "20", "--infinity-pixels", "30000"))
    print("Responsive help and playback cards passed", flush=True)
    run("impossible_wireframe", ("--recipe", "9", "--scene", "8"))
    run("impossible_wireframe", ("--recipe", "9", "--scene", "8", "--cpu-effects"))
    run("impossible_wireframe", ("--recipe", "1", "--scene", "10"))
    numeric = subprocess.run([str((options.build / ("gpu_deform_tests" + suffix)).resolve()),
                              str(Path(__file__).resolve().parents[1] / "engine/shaders")],
                             capture_output=True, text=True, timeout=60)
    if numeric.returncode:
        raise RuntimeError(numeric.stdout + numeric.stderr)
    print(numeric.stdout.strip(), flush=True)
    for scene in range(26):
        extra = ["--silent", "--infinity-scene", str(scene), "--infinity-pixels", "30000",
                 "--validate-output", "--bpm", "120", "--beat-division", "4",
                 "--pulse-amount", "2", "--flash-amount", "2", "--fixed-step", "0.125"]
        if scene == 0:
            extra.append("--validate-targets")
        if scene < 4:
            extra.append("--validate-ants")
        run("hold_infinity", extra, f"HDR output validation: scene {scene:02} finite")
        print(f"Infinity scene {scene:02}: finite HDR passed", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
