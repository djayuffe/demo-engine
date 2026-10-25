#!/usr/bin/env python3
"""Alternate CPU/GPU paths; record CPU work, upload traffic and optical GPU timings."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=Path("build"))
    parser.add_argument("--out", type=Path, default=Path("build/gpu-offload"))
    parser.add_argument("--frames", type=int, default=180)
    parser.add_argument("--repeats", type=int, default=2)
    args = parser.parse_args()
    if args.frames < 30 or args.repeats < 1:
        parser.error("Use at least 30 frames and one repeat")
    args.out.mkdir(parents=True, exist_ok=True)
    rows = []
    suffix = ".exe" if sys.platform == "win32" else ""
    common = ["--frames", str(args.frames), "--fixed-step", "0.016667", "--no-ticker", "--no-vsync", "--stats"]
    for workload in ("vertex", "instances", "traffic"):
        for repeat in range(args.repeats):
            for cpu in ((True, False) if repeat % 2 == 0 else (False, True)):
                if workload == "traffic":
                    binary = "hold_infinity"
                    extra = ["--silent", "--infinity-scene", "1", "--infinity-pixels", "30000",
                             "--window", "640x400", "--gpu-profile"] + (["--cpu-ants"] if cpu else [])
                else:
                    binary = "impossible_wireframe"
                    extra = ["--scene", "10", "--recipe", "21" if workload == "vertex" else "1",
                             "--window", "320x200", "--quality", "0.5", "--no-post"]
                    if cpu:
                        extra.append("--cpu-effects")
                command = [str((args.build / (binary + suffix)).resolve()), *common, *extra]
                run = subprocess.run(command, capture_output=True, text=True, timeout=180)
                log = run.stdout + run.stderr
                mode = "cpu" if cpu else "gpu"
                (args.out / f"{workload}-{repeat}-{mode}.log").write_text(log)
                if run.returncode or any(word in log.lower() for word in ("fallback (log once)", "sw vertex", "software vertex", "unavailable")):
                    raise RuntimeError(log)
                work = re.search(r"CPU work: frames=(\d+) avg=([\d.]+) ms max=([\d.]+) ms gpu-effect-frames=(\d+) gpu-effect-vertices=(\d+)", log)
                uploads = re.search(r"Mesh uploads: vertices=(\d+) indices=(\d+) bytes=(\d+)", log)
                if not work or not uploads:
                    raise RuntimeError("Missing counters: " + log)
                row = dict(workload=workload, repeat=repeat, mode=mode, frames=int(work[1]),
                           cpu_ms=float(work[2]), deferred_frames=int(work[4]), deferred_vertices=int(work[5]),
                           vertex_uploads=int(uploads[1]), index_uploads=int(uploads[2]), upload_bytes=int(uploads[3]))
                if workload == "traffic":
                    timing = re.search(r"GPU scene 01: n=(\d+) avg=([\d.]+) ms p95=([\d.]+) ms", log)
                    traffic = re.search(r"GPU traffic: splats=(\d+) cpu-texture-uploads=(\d+)", log)
                    if not timing or not traffic:
                        raise RuntimeError("Missing GPU traffic counters: " + log)
                    row.update(gpu_samples=int(timing[1]), gpu_ms=float(timing[2]), p95_ms=float(timing[3]),
                               splats=int(traffic[1]), texture_uploads=int(traffic[2]))
                elif not cpu and row["deferred_frames"] < args.frames // 2:
                    raise RuntimeError("Too few frames exercised GPU effects")
                rows.append(row)
                print(json.dumps(row), flush=True)
    (args.out / "results.json").write_text(json.dumps(rows, indent=2) + "\n")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
