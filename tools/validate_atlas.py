#!/usr/bin/env python3
"""Validate Atlas provenance, GPU output, temporal reset and presentation controls."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', default='build/hold_infinity')
    parser.add_argument('--out', default='build/atlas-controls')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    manifest = json.loads((root / 'demos/atlas/source.json').read_text())
    for name, expected in manifest['files'].items():
        actual = hashlib.sha256((root / 'demos/atlas/reference' / name).read_bytes()).hexdigest()
        if actual != expected:
            raise SystemExit('Modified archive reference: ' + name)
    print('All archived source hashes match', flush=True)
    binary = Path(args.binary).resolve()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)

    def run(name, scene, options=(), frames=45):
        image = out / (name + '.png')
        command = [str(binary), '--silent', '--no-ticker', '--bpm', '132',
                   '--infinity-scene', str(scene), '--infinity-eye', '0', '0',
                   '--infinity-pixels', '120000', '--window', '640x360',
                   '--frames', str(frames), '--fixed-step', '0.133333',
                   '--validate-output', '--screenshot', str(image)] + list(options)
        result = subprocess.run(command, capture_output=True, text=True, timeout=120)
        log = result.stdout + result.stderr
        (out / (name + '.log')).write_text(log)
        if result.returncode or not image.is_file() or 'HDR output validation:' not in log:
            raise SystemExit(name + ' failed:\n' + log)
        print(name + ': passed', flush=True)
        return image.read_bytes()

    for scene in range(16, 27):
        run(f'scene-{scene}', scene)
    for scene in (16, 20, 23, 24, 25, 26):
        run(f'portrait-eye-{scene}', scene, ('--window', '360x640', '--infinity-eye', '.8', '-.7'))
    for scene in (16, 18, 20, 23, 24, 26):
        a = run(f'paused-short-{scene}', scene, ('--atlas-paused',), 4)
        b = run(f'paused-long-{scene}', scene, ('--atlas-paused',), 30)
        if a != b:
            raise SystemExit(f'Pause drift in scene {scene}')
    for scene in (18,19,22):
        run(f'grazing-relief-{scene}',scene,('--infinity-orbit','.5','1.1'))
    for scene in (12,13,14,15,16,17,20,23,24,25,26):
        run(f'orbit-{scene}',scene,('--infinity-orbit','.65','.4'))
    run('edge-raster-orbit',17,('--atlas-edge-raster','--infinity-orbit','.6','.4'))
    run('edge-raster-zero-exposure',17,('--atlas-edge-raster','--atlas-exposure','0'))
    run('uncached-h3',20,('--atlas-uncached',))
    run('reduced-motion', 21, ('--atlas-reduced-motion', '--atlas-flash-gain', '0', '--atlas-exposure', '.7'))
    run('zero-exposure', 20, ('--atlas-exposure', '0'))
    for option, value in (('--atlas-exposure', '-1'), ('--atlas-exposure', 'nan'),
                          ('--atlas-flash-gain', '1.01'), ('--atlas-flash-gain', 'inf')):
        result = subprocess.run([str(binary), option, value], capture_output=True, text=True, timeout=15)
        if result.returncode == 0:
            raise SystemExit('Accepted invalid control: ' + option + ' ' + value)
    print('Atlas source, parallax, portrait, pause, exposure and temporal HDR checks passed', flush=True)


if __name__ == '__main__':
    main()
