#!/usr/bin/env python3
"""Run all 12 native GPU scenes; saves captures and fails on shader/FBO/draw errors."""
import argparse
from pathlib import Path
import subprocess


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',default='build/hold_infinity')
    parser.add_argument('--out',default='build/infinity-smoke')
    args=parser.parse_args()
    binary=Path(args.binary).resolve();out=Path(args.out).resolve();out.mkdir(parents=True,exist_ok=True)
    for scene in range(12):
        run=subprocess.run([str(binary),'--infinity-scene',str(scene),'--infinity-pixels','160000',
                            '--window','640x360','--silent','--infinity-eye','0','0','--frames','6','--fixed-step','0.033333','--no-ticker',
                            '--screenshot',str(out/f'{scene:02d}.png')],capture_output=True,text=True,timeout=120)
        (out/f'{scene:02d}.log').write_text(run.stdout+run.stderr)
        if run.returncode or not (out/f'{scene:02d}.png').is_file():
            raise SystemExit(f'Scene {scene} failed:\n{run.stdout}{run.stderr}')
        print(f'Scene {scene:02d}: passed',flush=True)
    print('All 12 optical scenes rendered successfully')


if __name__=='__main__':main()
