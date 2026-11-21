# System information and desktop overlay design

[中文](plugin-design.md) · [UI](UI-SPEC.en.md) · [Tests](TEST-PLAN.en.md)

Date: 2026-11-21. Version: 1.6.0. The native monitor is
installed and running. Build, logic, independent rendering and dual-monitor
interaction tests pass. Requirements below retain their individual acceptance
status in the test plan.

## Architecture

One sampler publishes snapshots shared by consumers. Unknown values are
unavailable, never fabricated zeroes. Core logic handles deltas, units,
timestamps, stale state and layout. hwinfo, IP Helper and PDH provide hardware
information, network rates and GPU/disk utilization.

Qt 6.12 Core/Gui/Qml/Quick manages protocol, event loop, settings and the
settings window; the window uses only QtQuick, QtQuick.Controls.Basic and
QtQuick.Layouts from the host's shared Qt contract. The host card stores the
common settings, while layout, text, charts, material and interaction values go
to `advanced-settings.json` in the plugin data directory. NativeOverlay converts
snapshots into DirectWrite/Direct2D content. The independent
[liquid-glass-cpp](https://github.com/ra9fael/liquid-glass-cpp) library owns Win32
windows, D3D11/DXGI capture, HLSL materials, DirectComposition and edge animation,
without Qt or WinShun dependencies. Both renderer and hwinfo are pinned submodules.

`plugin.json` declares ra9fael.sysinfo, the sys command, shared Qt 6.12 and
`ui`/`clipboard:write` permissions. The overlay defaults off and restores the
user's explicitly enabled state. Builds use Release Qt.

`Sample` and `Reading` hold values, status and six metric histories. Network
deltas use adapter identity. `intervalMs` ranges from 100 to 5000 ms, default
1000 ms. History retains 60 samples; its time span follows the interval. Capture
and animation are scheduled independently of metrics.

## Requirements

| ID | Priority | Observable requirement |
| --- | --- | --- |
| SYS-F01 | P0 | sys returns summaries without side effects; metrics can be copied. |
| SYS-F02 | P0 | Details group hardware fields, distinguish missing values and support multiple adapters. |
| SYS-F03 | P0 | Consumers agree for a timestamp without duplicate samplers. |
| SYS-F04 | P0 | Overlay defaults off; show/hide, lock and click-through remain controllable. |
| SYS-F05 | P0 | Persist metric, layout and monitor preferences; recover visible placement after monitor loss. |
| SYS-F06 | P0 | Hidden consumers reduce sampling; resets and resume do not create false rates. |
| SYS-F07 | P0 | No elevation, network requests, user-file scanning or unsolicited export. |
| SYS-F08 | P0 | Disable and host exit release windows, workers and counters. |
| SYS-F09 | P0 | Themes, scaling, localization, keyboard and accessibility work without stealing focus. |
| SYS-F10 | P1 | Optional fullscreen hiding has documented application coverage. |
| SYS-F11 | P1 | Explicit diagnostic export omits personal identifiers by default; cancellation writes nothing. |
| SYS-F12 | P0 | Timestamps and status distinguish stale/unavailable data; one failure preserves healthy metrics. |

The first rate sample establishes a baseline. Counter resets require a new valid
delta. Driver availability determines which GPU fields can be shown.

## Interaction and data

The overlay supports centered normal/compact grids, inline/wash/dotted/bars
histories, shared or per-metric colors, liquid/frosted/square glass and
system/light/dark themes. Dragging saves position and monitor name. Four-edge
collapse keeps a glass half-circle; hover wakes it even during click-through.
Right click opens no menu. The animation setting controls dynamic responses.

Permissions are `ui` and `clipboard:write`. Collection does not elevate or
modify system settings. Preferences persist; sample history does not. Export
is not implemented. See the README and UI specification for setting syntax.

## Failure behavior and acceptance

| Trigger | Behavior or acceptance target |
| --- | --- |
| Unsupported driver/PDH field | Mark only that metric unavailable; retain healthy values. |
| Counter reset/resume | Rebuild a valid rate baseline. |
| Monitor loss/DPI change | Keep the overlay visible; validate preference restoration. |
| Stale data | Expose timestamp/status rather than presenting cached data as live. |
| Blocking source at exit | Verify worker cancellation and host process-tree cleanup. |

Deliverables include source, manifest, pinned dependencies, tests, bilingual
documentation and a wsplugin package. The [test plan](TEST-PLAN.en.md) separates
core, wiring, host, desktop and performance acceptance.

Verified: full build, plugin logic, GPU/desktop rendering, four-direction
click-through hover, dual-monitor movement and restart restoration, real sampling
intervals, and installed enabled/running state. Short probes measured CPU, GPU
timestamps, draw duration and draw intervals. See [material validation](docs/native-material-validation.md)
and [desktop integration](docs/native-integration-results.md).

## Limitations

Native mixed-DPI, external recorders, independent dynamic backgrounds, physical
drag performance traces, full accessibility and host-exit process-tree acceptance
remain outstanding. Export and automatic fullscreen hiding are not implemented.
Driver capabilities vary between devices. Short probe results are not a complete
host resource budget.
