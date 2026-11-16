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
```

The plugin-only configuration does not configure hwinfo or liquid-glass-cpp.
Assertions remain enabled for the logic test executable in Release builds.

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
| LOGIC-03 | Plugin | Percent and byte-rate values preserve number/unit formatting | test_settings |
| LOGIC-04 | Plugin | Manifest's 30 setting keys match the settings model | test_settings |
| LOGIC-05 | Plugin | Settings clamp invalid ranges and emit only changed fields | test_settings |
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
