# Independent plugin and UI tests

## Ownership

`third_party/liquid-glass-cpp` is a Git submodule. Its `Window` owns Win32,
D3D11, desktop capture, Gaussian blur, material shaders, DirectComposition,
transparent corners and edge-reveal rendering. Its API uses standard C++ and
Windows types, with no Qt or sysinfo dependency.

`native_overlay.cpp` is the Qt adapter and application content renderer.
It converts Qt geometry/images, draws metric text and histories through the
library's D2D callback, and forwards interaction events to `sysinfo_ui.cpp`.
Sampling, settings, protocol, persistence and monitor placement stay in sysinfo.

## Independent commands

Use a Visual Studio developer shell from the `ra9fael.sysinfo` directory:

```powershell
cmake -S third_party/liquid-glass-cpp -B build/liquid-glass-cpp -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/liquid-glass-cpp
ctest --test-dir build/liquid-glass-cpp --output-on-failure

cmake -S . -B ../build/sysinfo-logic -G Ninja -DCMAKE_BUILD_TYPE=Release -DSYSINFO_BUILD_PLUGIN=OFF -DCMAKE_PREFIX_PATH=D:/tools/Qt/6.12.0/msvc2022_64
cmake --build ../build/sysinfo-logic
ctest --test-dir ../build/sysinfo-logic -L logic --output-on-failure

qmltestrunner -platform offscreen -input tests -o qml-junit.xml,junitxml
```

The plugin-only configuration does not configure hwinfo or liquid-glass-cpp.
Assertions remain enabled for the logic test executable in Release builds.
The QML suite needs no build step: it loads `SettingsWindow.qml` from the source
tree against a stub bridge, so it runs without the plugin host.

## Test cases

| Case | Layer | Input and expected result | CTest label |
|---|---|---|---|
| `render-geometry-reveal` | Library GPU | Reveal geometry, anchors, work-area clipping and animation curves | `synthetic` |
| `render-fixed-resize` | Library GPU | Asymmetric gradient keeps orientation and sampling through resize | `synthetic` |
| `render-fixed-blur` | Library GPU | Neutral checker blur stays neutral and smooths spatial contrast | `synthetic` |
| `render-edge-capsule-short` | Library GPU | Short panels constrain edge capsules to a semicircle | `synthetic` |
| `render-edge-capsule-sides` | Library GPU | Collapsed capsules render and restore on all four edges | `synthetic` |
| `render-click-through-edges` | Library GPU | Hover expands and pointer exit collapses click-through edges | `synthetic` |
| `render-hit-testing` | Library GPU | Click-through and collapsed panels expose the underlying window | `synthetic` |
| `render-capture-mapping-remote-scale` | Capture mapping | Half-resolution desktop textures preserve crop scale and coverage | `synthetic` |
| `render-capture-mapping-monitor-change` | Capture mapping | Changed monitor origin and dimensions keep crops within the frame | `synthetic` |
| `render-desktop-capture-visible` | Desktop capture | Visible and excluded capture modes omit panel content | `desktop` |
| `render-desktop-capture-resize` | Desktop capture | Resizing recaptures the desktop and keeps corners transparent | `desktop` |
| `render-capture-geometry-live` | Desktop capture | Capture geometry follows the current monitor rectangle | `desktop` |
| `render-capture-lock` | Session | Lock preserves the last backdrop and releases capture | `desktop` |
| `render-capture-unlock` | Session | Unlock recreates duplication and samples the current desktop | `desktop` |
| `render-capture-disconnect-reconnect` | Session | Remote disconnect/connect rebuilds capture for the current output | `desktop` |
| `render-capture-loss` | Capture recovery | `DXGI_ERROR_ACCESS_LOST` recreates duplication | `desktop` |
| `render-capture-invalid-call` | Capture recovery | `DXGI_ERROR_INVALID_CALL` recreates duplication | `desktop` |
| `render-capture-not-found` | Capture recovery | `DXGI_ERROR_NOT_FOUND` recreates duplication | `desktop` |
| `render-capture-backoff` | Capture recovery | Unavailable capture waits between attempts | `desktop` |
| `render-capture-display-change` | Session | Display changes recreate duplication and sample current pixels | `desktop` |
| `render-capture-session-repeat` | Session | Repeated lock/unlock cycles recreate capture once per unlock | `desktop` |
| LOGIC-01 | Plugin | Counter first sample/reset returns Sampling, normal delta yields rate | test_settings |
| LOGIC-02 | Plugin | Stale transition occurs at the configured age boundary | test_settings |
| LOGIC-03 | Plugin | Percent formatting and compact byte rates (K/M/G carry, one decimal below 100) | test_settings |
| LOGIC-04 | Plugin | Manifest's 9 setting keys and the model's 43 advanced keys define the settings surface | test_settings |
| LOGIC-05 | Plugin | Settings clamp invalid ranges and emit only changed fields | test_settings |
| LOGIC-06 | Plugin | Fresh settings default to the host's Alibaba PuHuiTi 3.0, preserved when a family is saved | test_settings |
| LOGIC-07 | Plugin | Label mode accepts text/icon; each metric id maps to a fixed Phosphor glyph | test_settings |
| QML-01 | Plugin UI | Slider drags commit on release only; interval floors at 100 ms | tst_SettingsWindow.qml |
| QML-02 | Plugin UI | A preset previews the full layout including metric order and selected state; Apply saves the draft, closing discards it | tst_SettingsWindow.qml |
| QML-03 | Plugin UI | Metric labels read CPU/GPU/UP/DN/MEM/DISK in English and 处理器/显卡/上行/下行/内存/磁盘 in Chinese | tst_SettingsWindow.qml |
| QML-04 | Plugin UI | Icon mode maps all six metrics to Phosphor glyphs in combos and the preview frame | tst_SettingsWindow.qml |
| INT-01 | Adapter | Labels/values/units/tiny/wash remain readable at 100/125/150/200% | Desktop visual verification |
| INT-02 | Window | Two-monitor drag persists the destination screen and position | Desktop integration |
| INT-03 | Window | Edge collapse/reveal works on each monitor and after resizing | Desktop integration |
| INT-04 | Capture | captureVisible on/off and moving background do not introduce self text | Desktop integration |
| INT-05 | Composition | Desktop corners reveal the actual underlying window after resize | Desktop integration |

## Which cases to run

Each library case is registered as a separate `render-<case>` CTest test. CI
runs the complete suite; labels allow focused local runs and a desktop-only
escape hatch.

| Change | Command |
|---|---|
| Capture crop mapping or monitor geometry | `ctest --test-dir build/liquid-glass-cpp -R 'render-capture-(mapping|geometry-live)' --output-on-failure` |
| Session and duplication recovery | `ctest --test-dir build/liquid-glass-cpp -R 'render-capture-(lock|unlock|disconnect-reconnect|loss|invalid-call|not-found|backoff|display-change|session-repeat)' --output-on-failure` |
| Synthetic GPU cases | `ctest --test-dir build/liquid-glass-cpp -L synthetic --output-on-failure` |
| Skip desktop-dependent cases | `ctest --test-dir build/liquid-glass-cpp -LE desktop --output-on-failure` |
| Full gate | `ctest --test-dir build/liquid-glass-cpp --output-on-failure` |

## Repository setup

The submodule URL is `../liquid-glass-cpp`, a sibling repository relative to
the plugin repository. Commit library changes in the submodule first, then
update the parent repository's pinned submodule commit.
