"""Measure native frame timings and process CPU on the interactive desktop."""
import json
import os
from pathlib import Path
import statistics
import subprocess
import tempfile
import time
import psutil
import desktop_integration as desktop

root = Path(__file__).resolve().parents[1]
env = os.environ.copy()
env['PATH'] = r'D:\tools\Qt\6.12.0\msvc2022_64\bin;' + env['PATH']
env['SYSINFO_MEASURE_FRAMES'] = '1'
results = []
with tempfile.TemporaryDirectory() as temporary:
    folder = Path(temporary)
    for visible in (False, True):
        for scenario in ('stationary', 'hover', 'moving'):
            settings = folder / 'settings.json'
            settings.write_text(json.dumps(dict(captureVisible=visible, edgeHide=True, clickThrough=True)))
            with (folder / 'frames.txt').open('w+') as log:
                process = subprocess.Popen([str(root / 'sysinfo.exe'), '--probe', '--settings-json', str(settings)], env=env, stderr=log)
                try:
                    time.sleep(2)
                    monitor = psutil.Process(process.pid)
                    hwnd = desktop.window_for(process.pid)
                    before = sum(monitor.cpu_times()[:2])
                    started = time.perf_counter()
                    for step in range(80):
                        if scenario == 'hover':
                            x, y, width, height = desktop.rect(hwnd)
                            desktop.u.SetCursorPos(x + width - 2 if step % 20 < 10 else x + width + 100, y + height // 2)
                        elif scenario == 'moving':
                            assert desktop.u.SetWindowPos(hwnd, 0, 100 + step * 3, 100 + step % 10, 0, 0, 0x0015)
                            assert desktop.rect(hwnd)[2] > 0
                        time.sleep(.1)
                    elapsed = time.perf_counter() - started
                    cpu = (sum(monitor.cpu_times()[:2]) - before) / elapsed * 100
                finally:
                    process.terminate()
                    process.wait()
                log.seek(0)
                rows = [list(map(float, line.split()[1:])) for line in log if line.startswith('FRAME ')]
                rows = rows[60:]
                def summary(values):
                    values = sorted(values)
                    return dict(median=round(statistics.median(values), 3), p95=values[int((len(values)-1)*.95)]) if values else None
                results.append(dict(captureVisible=visible, scenario=scenario, cpuOneCorePercent=round(cpu, 2),
                    frameMs=summary([r[0] for r in rows]), captureMs=summary([r[1] for r in rows]),
                    gpuMs=summary([r[2] for r in rows if r[2] >= 0]), intervalMs=summary([r[3] for r in rows])))
                print(json.dumps(results[-1]), flush=True)
