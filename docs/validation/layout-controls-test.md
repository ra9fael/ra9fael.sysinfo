# Layout controls validation

## Package

`dist/manual-test-20261121-layout-msvc/ra9fael.sysinfo-1.6.0.wsplugin`, 878460
bytes, SHA256
`fd1e64a4063a8d217761be1dfe40e95fdb8d00256ecc4b10c6b87fffa04c41cd`.

Installed over the running 1.6.0 copy by replacing `plugin.json`, the two
READMEs and `sysinfo.exe` in
`%LOCALAPPDATA%\WinShun\plugins\ra9fael.sysinfo`. `WinShun.exe` was not
restarted. `plugins/ra9fael.sysinfo.data/advanced-settings.json` and
`window.json` were left byte-identical, so the saved position, material and
metric choices survived.

## Build

- `sysinfo.exe` and `sysinfo_tests` built in Release with Qt 6.12.0 MSVC 2022 x64.
- `sysinfo_core` passed; `sysinfo tests: all passed`.
- `qmllint SettingsWindow.qml` reports no errors and no warnings beyond the
  file's pre-existing unqualified-access ones.

## Number box and slider

A Qt Quick harness drives the real `SettingsWindow.qml` against a stub bridge
and delivers keys through the window system interface:

- Typing a value and pressing Enter issues exactly one edit, carrying the typed
  number, and the slider moves to it.
- A typed value beyond the range clamps to the row maximum and the number box
  shows the clamped value.
- Escape reverts uncommitted text to the committed value and issues no edit.
- The scale row submits through the same path.

## Layout presets

- The compact bar preset issues one layout apply with columns 6, cell width 76,
  cell height 28, padding 4 and tiny true, leaving metrics and scale alone.
- Restore default layout issues one apply returning columns 3, cell width 120
  and scale 100, leaving label language alone.
- The actual size readout renders the panel metrics the backend computes.

## Settings behaviour

- `advanced-settings.json` now carries 42 advanced keys.
- `labelLanguage` accepts `follow`, `zh` and `en`, defaults to `follow`, rejects
  other values and round-trips through JSON.
- `tiny` no longer overrides the cell height, so the height slider works in
  every layout; the plugin window's layout page applies scale to text, cells,
  padding and the glass frame together.

## Pending

- Rendering evidence for the new layout page in an installed host: preset
  selection, the actual size readout and the label language switch still need a
  screenshot from a real desktop session.
