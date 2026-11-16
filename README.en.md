# ra9fael.sysinfo

[中文](README.md)

Windows system queries and a native liquid-glass desktop monitor. Version
`1.4.0-dev` is in development and installed in WinShun, confirmed enabled and running.

## Desktop monitor

CPU, memory, GPU, disk activity, upload and download share a configurable grid.
Both normal and compact modes support centered text and history charts.

- Cell order: `cpu,memory,gpu;disk,netUp,netDown` creates two rows and three columns.
  Semicolons or newlines separate rows; commas separate columns; `cpu,,memory`
  preserves a blank cell.
- Chart shape: `off`, `inline` lines, `wash` areas, `dotted` dots or `bars` columns.
  `wavePlacement` independently selects `below` or `background`; `auto` uses the
  shape default (wash behind text, others below).
- Below-text height: `waveHeight=0` uses available space automatically. Rows of
  28/36/40/56px allocate 6/12/16/32px. Custom heights of 4–136px are capped to
  available space, with text at the top and charts at the bottom. `waveOpacity`
  is 0–100% (default 45%); `waveDots` selects `filled` or `trace` dots.
  A 40px row with automatic chart height is recommended.
- Wave colors: one `#RRGGBB` for all metrics, or six comma-separated colors in
  CPU, memory, GPU, disk, upload, download order, independent of cell order.
- Fonts: configurable family and 70–180% size. Measured long text shrinks to fit.
- `tiny`: single-line text, 28px rows, 4px outer padding and automatic 6px bottom charts.
  Background charts need no extra height. Cell width, blank cells and label/value/unit switches
  are preserved. Large text shrinks to the fixed row height. Corner radius is capped
  at 10px; all dimensions follow the overall scale.
- Theme: `system` follows Windows; `light` and `dark` set glass tint and text color.
- Glass: `liquid`, `frosted`, or square-embossed `square`. Tile size is 8–80 pixels;
  refraction controls its strength.
- Interaction: drag, lock, click-through, four-edge half-circle collapse and hover
  expansion. Hover wake-up works with click-through enabled. Right click opens no
  menu. Position and monitor name persist.
- Motion: local pointer refraction, eased pressing and drag flow, controlled by
  the animation setting.

`intervalMs` controls metric and history sampling: 500–5000 ms, default 1000 ms.
History retains 60 samples, so its time span changes with the interval. Glass
capture and animation are scheduled independently; a 16 ms timer does not imply
60 fps.

`captureVisible` allows the overlay to appear in captures. When enabled,
background acquisition temporarily excludes the overlay, synchronizes DWM and
then restores capture visibility.

## Architecture and build

The plugin uses the WinShun SDK, C++23 and Qt 6.12 Core/Gui for protocol, event
loop, settings and sampling. The overlay uses Win32 HWND, D3D11/DXGI,
DirectComposition, HLSL and DirectWrite/Direct2D.

Dependencies are pinned Git submodules:

- [liquid-glass-cpp](https://github.com/ra9fael/liquid-glass-cpp) owns windows,
  backdrop capture, materials and edge animation without Qt or WinShun.
  Attribution is in its `THIRD-PARTY-NOTICES.md`.
- [hwinfo](https://github.com/lfreist/hwinfo) 2.0.0, commit
  `acdc2eb4a9109b4ea6bbe207e03d9da33bc16fab`, MIT. Its license is retained at
  `third_party/hwinfo/LICENSE`.

Run in a Visual Studio developer terminal with the local Qt 6.12 MSVC prefix:

```powershell
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<Qt installation>"
cmake --build build
ctest --test-dir build --output-on-failure
```

Put Qt's `bin` directory on PATH for tests. Render tests require an interactive
Windows desktop and a D3D11 device. The executable is written to the source directory.

## Validation and permissions

Permissions are `clipboard:write` and `ui`. Collection does not access the network,
launch external processes, scan user files, elevate or change system settings.
hwinfo, Win32 IP Helper and PDH provide hardware information, network rates and
GPU/disk utilization; availability depends on the system and driver.

- [Functional design](plugin-design.en.md)
- [UI specification](UI-SPEC.en.md) · [Test plan](TEST-PLAN.en.md)
- [Native desktop integration](docs/native-integration-results.md)
- [Material, sampling and performance validation](docs/native-material-validation.md)

Verified: sampling intervals, four-direction click-through hover, dual-monitor
movement and position restoration. Short probes measured capture-visible median
draw duration near 10 ms, draw interval near 33 ms and median GPU draw time of
0.06–0.08 ms. See the validation record for conditions and boundaries.

## Limitations

Both available monitors report 96 native DPI; native mixed-DPI remains untested.
External recorders, independent dynamic-background workloads, physical-drag
performance traces and full accessibility acceptance remain outstanding. The
renderer is public and independently cloned; a complete cross-machine plugin
build is unverified. Export and automatic fullscreen hiding are not implemented.
Sensor fields are not guaranteed on every device.
