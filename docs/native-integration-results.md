# Native overlay integration

Version: 1.4.0-dev. Renderer: `dbde6ec`. Updated: 2026-11-16.

## Verified behavior

- Independent GPU tests pass orientation, sampling, neutral blur, repeated
  resize and transparent render-target corners.
- Desktop capture follows a stationary background through gray levels
  220, 40 and 220. Both captureVisible modes retain the underlying pixels
  without capturing an opaque red rectangle drawn by the content callback.
- Actual desktop corner pixels match the background in both capture modes.
- Content snapshots at 100%, 125%, 150% and 200% retain proportional text and
  layout. Tiny, wash, labels-only and values-only configurations render.
- Native pointer events move the panel between BELLATOR N27Q and DELL U2722DX.
  The persisted coordinates and monitor names match each destination.
- Right click opens no menu. All four edges support half-circle collapse and
  hover expansion, including click-through pointer entry and delayed exit.
- Plugin logic tests pass with assertions enabled. The complete plugin builds.
- WinShun reports the installed 1.4.0-dev plugin enabled and running.

The test sources are `third_party/liquid-glass-cpp/tests/render_tests.cpp` and
`tests/desktop_integration.py`.
Content snapshots are in `build/glass-reference-analysis/`.

## Limitations

The reference shader techniques are implemented, but side-by-side visual parity
with a running imgui-liquid-glass demo has not been established. Capture-visible
testing covers this machine's desktop duplication path, not external recorders.
Native mixed-DPI transitions require monitors with different system DPI values.
Both monitors in the current desktop report 96 DPI.

The renderer is published at https://github.com/ra9fael/liquid-glass-cpp and the
plugin submodule uses that HTTPS URL. Independent cloning passed. Complete
cross-machine plugin builds remain unverified.

## Extended desktop coverage

`tests/desktop_integration.py` selects the probe window by process ID and uses a
temporary settings and position directory. It checks all four edges on every
monitor: collapsed center hit testing, the activation strip, expanded hit testing
and stable window geometry. It then restarts the process on each monitor and
compares the restored rectangle and persisted screen name and coordinates.

Both DELL U2722DX and BELLATOR N27Q pass all four edges and restart restoration.
Restored rectangles are `(150, 180, 400, 140)` and `(-2410, 180, 400, 140)`.
The same cases pass with `QT_SCREEN_SCALE_FACTORS=1;1.5`. This exercises Qt's
per-screen coordinate scaling; it does not change native Windows monitor DPI.

Run from the workspace root:

```powershell
python ra9fael.sysinfo/tests/desktop_integration.py
python ra9fael.sysinfo/tests/desktop_integration.py --qt-mixed-scale
python ra9fael.sysinfo/tests/desktop_integration.py --require-mixed-dpi
```

The strict mixed-DPI command fails if two distinct native window DPI values are
not observed. The regular command explicitly reports that branch as unexercised.

The renderer desktop test passes resize widths 360, 300 and 240 in both
capture modes, checking actual desktop RGB at all four corners, visible red
content, backbuffer dimensions and capture luminance. Screen pixel assertions
temporarily allow capture so GDI can observe the content. The backdrop window
is explicitly shown, painted and ordered below the glass before DWM synchronization.

Four-direction half-circle tests pass transparent panel centers, opaque glass
interiors, transparent outer corners and restored expanded surfaces. Collapse
and expansion interpolate the glass shape in place and fade content; background
coordinates remain aligned with the desktop throughout the transition.

Inline, wash, dotted and bars render in the native content pass with configurable
per-metric colors. Normal and compact modes honor explicit rows and center text.
Cell dimensions follow settings. Desktop
layout checks pass 1, 2, 3 and 6 columns at cell size 140 by 40, yielding window
sizes 180 by 280, 320 by 160, 460 by 120 and 880 by 80.

The native timer schedules drawing every 16 ms independently of metric sampling.
Capture acquisition is nonblocking. Capture-excluded windows use dirty and move
regions to skip unrelated background copies, with a 250 ms refresh fallback.
Capture-visible windows retain the affinity-toggle synchronization path.
Luminance readback copies one pixel instead of the full background texture.
Short measurements found capture-visible median draw duration near 10 ms and
draw completion intervals near 33 ms. Sampling interval measurements, font and
material coverage, CPU/GPU results and their limitations are recorded in
[native-material-validation.md](native-material-validation.md).

The rebuilt package was installed and WinShun reports version 1.4.0-dev,
enabled=true and status=running, with clipboard:write and ui permissions.
