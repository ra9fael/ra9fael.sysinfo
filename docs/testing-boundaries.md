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

Use a Visual Studio developer shell. From the workspace root:

```powershell
cmake -S liquid-glass-cpp -B build/liquid-glass-cpp -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/liquid-glass-cpp
ctest --test-dir build/liquid-glass-cpp --output-on-failure

cmake -S ra9fael.sysinfo -B build/sysinfo-logic -G Ninja -DCMAKE_BUILD_TYPE=Release -DSYSINFO_BUILD_PLUGIN=OFF -DCMAKE_PREFIX_PATH=D:/tools/Qt/6.12.0/msvc2022_64
cmake --build build/sysinfo-logic
ctest --test-dir build/sysinfo-logic -L logic --output-on-failure

qmltestrunner -platform offscreen -input ra9fael.sysinfo/tests -o qml-junit.xml,junitxml
```

The plugin-only configuration does not configure hwinfo or liquid-glass-cpp.
Assertions remain enabled for the logic test executable in Release builds.
The QML suite needs no build step: it loads `SettingsWindow.qml` from the source
tree against a stub bridge, so it runs without the plugin host.

## Test cases

| ID | Layer | Input and expected result | Automation |
|---|---|---|---|
| UI-01 | Library GPU | Asymmetric gradient retains top/bottom and left/right orientation | render_tests |
| UI-02 | Library GPU | Interior samples align with source coordinates, without a second desktop blend | render_tests |
| UI-03 | Library GPU | All four rounded corners have zero alpha; center has full coverage | render_tests |
| UI-04 | Library GPU | Resize 240→360→300→240 preserves output dimensions and sampling | render_tests |
| UI-05 | Library GPU | Neutral checker blur stays neutral and smooths spatial contrast | render_tests |
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

## Repository setup

The submodule URL is `../liquid-glass-cpp`, a sibling repository relative to
the parent's remote (or local clone origin). In this workspace the sibling
repository is local. No GitHub remote or public repository has been created.
For distribution, host both repositories at sibling remote URLs and retain
the pinned submodule commit.
