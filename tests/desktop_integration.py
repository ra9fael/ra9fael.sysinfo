"""Run against an interactive desktop; all plugin state stays in a temporary directory."""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

u = c.WinDLL('user32', use_last_error=True)
u.SetProcessDpiAwarenessContext.argtypes = [w.HANDLE]
u.SetProcessDpiAwarenessContext(w.HANDLE(-4))
u.SendMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
u.SendMessageW.restype = c.c_ssize_t
u.GetWindowRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
u.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
u.GetClassNameW.argtypes = [w.HWND, w.LPWSTR, c.c_int]
u.GetDpiForWindow.argtypes = [w.HWND]
u.GetMonitorInfoW.argtypes = [w.HANDLE, c.c_void_p]

class MonitorInfo(c.Structure):
    _fields_ = [('size', w.DWORD), ('monitor', w.RECT), ('work', w.RECT),
                ('flags', w.DWORD), ('device', w.WCHAR * 32)]

def monitors():
    result = []
    callback = c.WINFUNCTYPE(w.BOOL, w.HANDLE, w.HDC, c.POINTER(w.RECT), w.LPARAM)
    @callback
    def visit(handle, dc, rect, param):
        info = MonitorInfo(size=c.sizeof(MonitorInfo))
        assert u.GetMonitorInfoW(handle, c.byref(info))
        result.append(info)
        return True
    u.EnumDisplayMonitors(None, None, visit, 0)
    return result

def window_for(pid):
    found = []
    callback = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    @callback
    def visit(hwnd, param):
        owner = w.DWORD()
        u.GetWindowThreadProcessId(hwnd, c.byref(owner))
        name = c.create_unicode_buffer(128)
        u.GetClassNameW(hwnd, name, len(name))
        if owner.value == pid and name.value == 'LiquidGlassCppWindow':
            found.append(hwnd)
        return True
    u.EnumWindows(visit, 0)
    return found[0] if found else None

def rect(hwnd):
    r = w.RECT()
    assert u.GetWindowRect(hwnd, c.byref(r))
    return r.left, r.top, r.right - r.left, r.bottom - r.top

def packed(x, y):
    return (x & 0xffff) | ((y & 0xffff) << 16)

def drag(hwnd, x, y):
    old = rect(hwnd)
    u.SetCursorPos(old[0] + 40, old[1] + 40)
    u.SendMessageW(hwnd, 0x200, 0, packed(40, 40))
    time.sleep(.3)
    u.SendMessageW(hwnd, 0x201, 1, packed(40, 40))
    u.SetCursorPos(x + 40, y + 40)
    u.SendMessageW(hwnd, 0x200, 1, packed(40, 40))
    u.SendMessageW(hwnd, 0x202, 0, packed(40, 40))
    time.sleep(.4)
    assert rect(hwnd)[:2] == (x, y), ('drag', rect(hwnd), x, y)

def edges(hwnd, area):
    width, height = rect(hwnd)[2:]
    cx = area.left + (area.right - area.left - width) // 2
    cy = area.top + (area.bottom - area.top - height) // 2
    for side, x, y, sx, sy in [
        ('left', area.left, cy, 2, height // 2),
        ('right', area.right - width, cy, width - 2, height // 2),
        ('top', cx, area.top, width // 2, 2),
        ('bottom', cx, area.bottom - height, width // 2, height - 2),
    ]:
        drag(hwnd, x, y)
        u.SetCursorPos(cx + width + 60, cy + height + 60)
        u.SendMessageW(hwnd, 0x2a3, 0, 0)
        time.sleep(1.2)
        assert u.SendMessageW(hwnd, 0x84, 0, packed(x + width // 2, y + height // 2)) == -1, side
        assert u.SendMessageW(hwnd, 0x84, 0, packed(x + sx, y + sy)) == 1, (side, 'strip')
        u.SetCursorPos(x + sx, y + sy)
        u.SendMessageW(hwnd, 0x200, 0, packed(sx, sy))
        time.sleep(.4)
        assert u.SendMessageW(hwnd, 0x84, 0, packed(x + width // 2, y + height // 2)) == 1, (side, 'reveal')
        assert rect(hwnd) == (x, y, width, height), (side, 'geometry drift')
        print('PASS edge', side, flush=True)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', type=Path, default=Path(__file__).resolve().parents[1] / 'sysinfo.exe')
    parser.add_argument('--qt-bin', default=r'D:\tools\Qt\6.12.0\msvc2022_64\bin')
    parser.add_argument('--qt-mixed-scale', action='store_true', help='Exercise Qt coordinate conversion with per-screen scale factors')
    parser.add_argument('--require-mixed-dpi', action='store_true')
    args = parser.parse_args()
    env = os.environ.copy()
    env['PATH'] = args.qt_bin + os.pathsep + env['PATH']
    if args.qt_mixed_scale:
        env['QT_SCREEN_SCALE_FACTORS'] = '1;1.5'
    screens = monitors()
    with tempfile.TemporaryDirectory(prefix='sysinfo-desktop-') as directory:
        data = Path(directory)
        settings = data / 'settings.json'
        settings.write_text(json.dumps({'edgeHide': True, 'position': 'free', 'displayTarget': 'saved'}))
        command = [str(args.exe), '--probe', '--settings-json', str(settings), '--probe-data', str(data)]
        def start():
            process = subprocess.Popen(command, env=env)
            for _ in range(60):
                hwnd = window_for(process.pid)
                if hwnd:
                    time.sleep(.6)
                    return process, hwnd
                time.sleep(.1)
            process.terminate()
            process.wait()
            raise AssertionError('probe window did not appear')
        process, hwnd = start()
        dpis = set()
        try:
            for screen in screens:
                area = screen.work
                x, y = area.left + 150, area.top + 180
                drag(hwnd, x, y)
                dpi = u.GetDpiForWindow(hwnd)
                dpis.add(dpi)
                print('monitor', screen.device, 'native DPI', dpi, 'rect', rect(hwnd), flush=True)
                edges(hwnd, area)
                drag(hwnd, x, y)
                expected = rect(hwnd)
                saved = json.loads((data / 'window.json').read_text())
                assert (saved['x'], saved['y']) == (x, y) and saved['screen'], saved
                process.terminate()
                process.wait()
                process, hwnd = start()
                assert rect(hwnd) == expected, ('restart', expected, rect(hwnd))
                assert json.loads((data / 'window.json').read_text()) == saved
                print('PASS restart', screen.device, expected, flush=True)
            if len(dpis) > 1:
                print('PASS native mixed-DPI', sorted(dpis))
            else:
                print('NOT EXERCISED native mixed-DPI: monitors have the same DPI', sorted(dpis))
                assert not args.require_mixed_dpi, 'two distinct native monitor DPIs are required'
        finally:
            process.terminate()
            process.wait()

if __name__ == '__main__':
    main()
