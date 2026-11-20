# Settings window validation

## Package

`dist/sysinfo-1.5.1-dev/ra9fael.sysinfo-1.5.1-dev.wsplugin`, SHA256
`92f87c6f5a0684b1530685f97c421351a799658b2f87fad816bd07d62c3b6ca5`.

Installed over the existing 1.5.0-dev copy with `ws plugin install`. WinShun
reports `1.5.1-dev`, `enabled: true`, `status: running`; the overlay window is
visible. The first start created
`plugins/ra9fael.sysinfo.data/advanced-settings.json` from the installed
configuration: Consolas, `cpu,memory,gpu;`, `shadow` 2, `waveOpacity` 100 (from
125), with the 35 advanced keys and no host keys.

## Build

- `sysinfo.exe` and `sysinfo_tests` built in Release with Qt 6.12.0 MSVC 2022 x64.
- `sysinfo_core` passed in 0.05 s.
- `liquid_glass_render` passed in 31–85 s across runs; the test timeout is 180 s.

## Window

The window is Qt Quick (`SettingsWindow.qml`, compiled into the executable),
using only QtQuick, QtQuick.Controls.Basic and QtQuick.Layouts from the host's
shared Qt contract.

Protocol run: `initialize` with `enabled: false`, then `showSettings` twice,
then `shutdown`.

- The first `showSettings` created one visible settings window.
- The second `showSettings` returned the same window handle; no second window appeared.
- No overlay window was created while `enabled` was false.
- stderr was empty: no QML component or binding warnings.
- stdout answered both `showSettings` calls with `{}` and `shutdown` with `{}`.

UI Automation against the same process:

- The window exposed the five General checkboxes and the tab controls.
- The Layout tab showed the six configured metric cells (CPU, disk, upload,
  memory, GPU, download).
- Changing the first cell to GPU updated the draft preview; Apply produced no
  QML warnings.
- Toggling “Show monitor” off hid the overlay; toggling it on showed it again.
- The Close button hid the window without ending the process.
- A second run with `locale: en` and a light `nativeLook` showed the English
  title, tab names and Close button, with stderr still empty.

## Settings behaviour

First run wrote `advanced-settings.json` with the 35 advanced keys and no host
keys. Legacy string values from the host were stored with their declared types:
`fontSize` 110, `dividers` true, `waveHeight` 16, `glass` 70, while `metrics`
and `waveColors` stayed strings.

A run using the installed configuration's values migrated `fontFamily`
Consolas, `metrics` `cpu,memory,gpu;`, `shadow` 2 and `waveOpacity` 125; the
out-of-range opacity was clamped to 100 and the rest persisted.

`sysinfo_tests` covers:

- first-run migration of legacy string-typed advanced values;
- saved advanced values taking precedence over later host-provided values;
- host settings changing without overwriting advanced values;
- immediate advanced writes on single edits;
- slider preview staying out of the file until commit;
- cancel restoring the pre-preview value;
- an unparseable advanced file being rewritten from the current values;
- the manifest comparison against the seven host keys.

## Pending

- The card's Settings button and the search settings row in an installed host.
- Palette and typography against a real host `nativeLook` message; the protocol
  run supplied one but no screenshot was taken.
- Click-through and hidden-overlay recovery from the settings window on the
  desktop with a real host.
