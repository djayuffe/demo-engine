#!/usr/bin/env python3
"""Extended native GPU audit: all scenes, full ray fields, target lifetime and boundary states."""
import argparse
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', default='build/hold_infinity')
    parser.add_argument('--out', default='build/infinity-validation')
    args = parser.parse_args()
    binary = Path(args.binary).resolve()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, str(Path(__file__).with_name('smoke_infinity.py')),
                    '--binary', str(binary), '--out', str(out / 'scenes')], check=True)

    def run(name, options, expected=None):
        command = [str(binary), '--validate-output', '--silent', '--no-ticker', '--bpm', '132', '--infinity-eye', '0', '0',
                   '--infinity-pixels', '120000', '--window', '640x360',
                   '--frames', '4', '--fixed-step', '0.0166667'] + options
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        log = result.stdout + result.stderr
        (out / (name + '.log')).write_text(log)
        if result.returncode or (expected and expected not in log):
            raise SystemExit(name + ' failed:\n' + log)
        print(name + ': passed', flush=True)
        return log

    run('target-storage', ['--infinity-scene', '0', '--validate-targets'], 'GPU target validation:')
    for scene in (10, 11):
        for curvature in ('0', '0.65', '1'):
            for window in ('640x360', '360x640'):
                run(f'rays-{scene}-{curvature}-{window}',
                    ['--infinity-scene', str(scene), '--grin-curvature', curvature,
                     '--window', window, '--validate-ray-state', '--infinity-sequential-integrator'], 'Ray-state validation:')
    for state in (31, 95, 127):
        run(f'optical-state-{state}', ['--instrument-state', str(state)])
    run('still-convergence', ['--infinity-scene', '5', '--still', '--frames', '205', '--gpu-profile'],
        'mipmaps=200')
    cycle=run('automatic-cycle', ['--frames', '260', '--fixed-step', '1'])
    if cycle.count('infinity scene 00:')<2:raise SystemExit('Automatic show did not complete its expanded cycle')
    print('Extended Infinity GPU validation passed', flush=True)


if __name__ == '__main__':
    main()
