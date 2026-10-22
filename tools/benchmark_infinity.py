#!/usr/bin/env python3
"""A/B native GPU workload at identical samples, resolution and deterministic beat input."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--binary',default='build/hold_infinity')
    ap.add_argument('--out',default='build/infinity-benchmark')
    ap.add_argument('--frames',type=int,default=120)
    ap.add_argument('--repeats',type=int,default=3)
    ap.add_argument('--scenes',default='0,5,7,10,11')
    ap.add_argument('--integration-only',action='store_true',help='Compare raster versus sequential RK4 with all other optimizations equal')
    a=ap.parse_args()
    if a.frames<30 or a.repeats<1:ap.error('At least 30 frames and one repeat required')
    scenes=[int(s) for s in a.scenes.split(',')]
    if any(s<0 or s>11 for s in scenes):ap.error('Scenes must be 0..11')
    root=Path(a.out).resolve();root.mkdir(parents=True,exist_ok=True);binary=Path(a.binary).resolve()
    if not shutil.which("ffmpeg"):print("FFmpeg unavailable: GPU timings only; image checks skipped",flush=True)
    rows=[]
    for scene in scenes:
        for rep in range(a.repeats):
            for reference in ([True,False] if rep%2==0 else [False,True]):
                mode='reference' if reference else 'optimized';tag=f'{scene:02d}-{rep}-{mode}'
                image=root/f'{tag}.png'
                cmd=[str(binary),'--infinity-scene',str(scene),'--infinity-pixels','120000','--window','640x360',
                     '--frames',str(a.frames),'--fixed-step','0.0166667','--no-ticker','--silent','--bpm','132',
                     '--no-vsync','--infinity-eye','0','0','--gpu-profile','--screenshot',str(image)]
                if scene==10 and not a.integration_only:cmd+=['--grin-curvature','0.65']
                if reference:cmd+=['--infinity-raster-integrator' if a.integration_only else '--infinity-reference']
                elif a.integration_only:cmd+=['--infinity-sequential-integrator']
                run=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
                log=run.stdout+run.stderr;(root/f'{tag}.log').write_text(log)
                if run.returncode:raise SystemExit(log)
                time=re.search(r'GPU scene \d+: n=(\d+) avg=([\d.]+) ms p95=([\d.]+) ms',log)
                work=re.search(r'frames=(\d+) ray-solves=(\d+) uniform-lookups=(\d+)',log)
                if not time or not work:raise SystemExit('Missing GPU measurements: '+tag)
                row={'scene':scene,'repeat':rep,'mode':mode,'samples':int(time[1]),'avg_ms':float(time[2]),
                     'p95_ms':float(time[3]),'ray_solves':int(work[2]),'uniform_lookups':int(work[3])}
                row['comparison']='integration' if a.integration_only else 'full'
                rows.append(row);print(json.dumps(row),flush=True)
            if shutil.which('ffmpeg'):
                compare=subprocess.run(['ffmpeg','-hide_banner','-i',str(root/f'{scene:02d}-{rep}-reference.png'),
                        '-i',str(root/f'{scene:02d}-{rep}-optimized.png'),'-lavfi','psnr','-f','null','-'],capture_output=True,text=True)
                if compare.returncode:raise SystemExit(compare.stderr)
                psnr=re.search(r'average:([\d.]+|inf)',compare.stderr)
                if not psnr:raise SystemExit('Missing image comparison')
                value=float(psnr[1]);print(f'scene {scene} image PSNR: {value} dB',flush=True)
                structural=subprocess.run(['ffmpeg','-hide_banner','-i',str(root/f'{scene:02d}-{rep}-reference.png'),
                    '-i',str(root/f'{scene:02d}-{rep}-optimized.png'),'-lavfi','ssim','-f','null','-'],capture_output=True,text=True)
                similarity=re.search(r'All:([\d.]+)',structural.stderr)
                if structural.returncode or not similarity:raise SystemExit('Missing structural image comparison')
                score=float(similarity[1]);print(f'scene {scene} SSIM: {score}',flush=True)
                # Stochastic multi-layer Fresnel paths amplify roundoff at grazing
                # crossings; require matching structure as well as signal level.
                if value<(35 if 4<=scene<=9 else 40) or score<.98:raise SystemExit('Unexpected visual difference')
                rows[-1]['image_ssim']=score
                rows[-1]['image_psnr_db']=None if value==float('inf') else value
    (root/'results.json').write_text(json.dumps(rows,indent=2)+'\n')


if __name__=='__main__':main()
