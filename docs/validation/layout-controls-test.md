# Layout controls validation

## Package

`dist/manual-test-20261121-alignment/ra9fael.sysinfo-1.6.0.wsplugin`, 1009647
bytes, containing `plugin.json`, the two READMEs, `sysinfo.exe` and
`licenses/phosphor.txt` (Phosphor's MIT license, required by the embedded font).

Installed over the running copy by replacing `plugin.json`, the two READMEs,
`sysinfo.exe` and `licenses/phosphor.txt` in
`%LOCALAPPDATA%\WinShun\plugins\ra9fael.sysinfo`. The previous executable was
renamed to `sysinfo.exe.old-20261121-171140` instead of deleted, so the running
process kept its image. `WinShun.exe` was not restarted; the host relaunched the
plugin at 17:11:42. `plugins/ra9fael.sysinfo.data/advanced-settings.json` and
`window.json` were untouched by the install, so saved layout, font and metric
choices survived. Every installed file hash-matches the staged package.

## Build

- `sysinfo.exe` and `sysinfo_tests` built in Release with Qt 6.12.0 MSVC 2022 x64.
- `sysinfo tests: all passed`; `qmllint SettingsWindow.qml` reports no errors and
  no warnings beyond the file's pre-existing unqualified-access ones.

## Probe run

The installed executable was also launched standalone against scratch data
directories, so live plugin data was untouched:

```
sysinfo.exe --probe --probe-data <tmp>\data --light --settings-json <overrides.json> --probe-app-dir <host app> --dump-native <tmp>\overlay.png
```

With text labels and `"labelLanguage": "en"`, names sit on the left edge and
readings on the right: `CPU 15 %` / `MEM 24 %`, `DISK 24 %` / `GPU 31 %`, and
`UP 41.4 B` / `DN 45 B`: the download label is `DN` and network rates use
B/KBps/M/G units, with the unit in a smaller face. With `"labelMode": "icon"`
the same six cells show the Phosphor glyphs for CPU, hard drives, memory,
graphics card, arrow up and arrow down, with values and units on the right. The
six-column compact-bar variant renders the same glyphs at 76 x 28 px without
clipping; stacked cells with `"sparklines": "off"` and Chinese names keep the
name above the reading.

Snapshots: `%TEMP%\opencode\probe-alignment\overlay100-4x.png`,
`overlay-bar-4x.png` and `overlay-stacked-3x.png`.

## Host font

The font family remains editable. A fresh `Settings` and a JSON with an empty
`fontFamily` both resolve to `Alibaba PuHuiTi 3.0`; a saved family is preserved.
The plugin loads `<host app>/fonts/AlibabaPuHuiTi-3-55-Regular.ttf` into Qt and,
because the native overlay draws with DirectWrite, builds a font collection from
the same file. The host app folder comes from the parent process image, since
plugins are installed outside it. Phosphor ships inside the executable as a Qt
resource, is extracted once to a temporary directory and is registered with
both Qt and DirectWrite, so icon mode needs no host-side file.

## Settings window suite

`tests/tst_SettingsWindow.qml` drives the real `SettingsWindow.qml` against a
stub bridge that records edits, previews and layout applies. It runs headless:

```
qmltestrunner -platform offscreen -input <repo>/tests -o <log>,junitxml
```

21 cases, 0 failures.

### Sampling interval

- `test_intervalSliderReachesFloor` drags the handle to the far left and
  releases: one edit with value 100, so the floor is 100 ms, not 500 ms.
- `test_intervalSliderCommitsOnRelease` presses the handle and moves it four
  times before releasing. While pressed, neither edits nor previews are
  recorded and the row value stays at 1000. On release, exactly one
  `intervalMs` edit carries the new value, the value is a multiple of 100 and
  the spin box follows.
- `test_intervalSpinClampsAndSubmitsOnce` types 250, then 20. The first issues
  one edit at 250; the second clamps to 100 and issues one edit, and the box
  shows the clamped text.

The floor is enforced in the manifest (`min: 100`), in `fromJson` (clamp to
100–5000), in the row's `from` and in the sampler's timer.

### Layout preview and apply

- `test_presetPreviewSwitchesArrangementAndSelection` clicks compact bar: one
  preview carries the six-metric arrangement as well as the size values, the
  cell-order combos and preview frame switch to that arrangement, and the
  selected state moves from standard grid to compact bar.
- `test_applyPersistsPreviewedPreset` clicks compact bar and then Apply: the
  single `applyLayout` carries the previewed metrics, columns, width, height,
  padding, scale and tiny flag, not the values the window opened with.
- `test_closeAfterPreviewDropsDraft` previews tall stack and closes: one
  `cancelPreview`, no layout apply.
- `test_layoutRowPreviewsWithoutSaving` types a cell width on the layout page:
  one preview, no edit and no layout apply, and the preview carries only
  `cellWidth`.
- `test_restorePreviewsStandardGrid` clicks restore: the preview drafts the
  standard grid with its metric arrangement and scale back at 100.
- `test_applySendsWholeDraft` clicks Apply layout: one `applyLayout` carrying
  metric order, columns, cell width and height, padding, scale and tiny
  together, so the store commits a single revision.
- `test_closeDropsDraft` closes the window: one `cancelPreview`, and the window
  hides instead of closing so the host can reopen it.

### Metric labels and icon mode

- `test_metricLabels` runs the cell-order combos under all four locale and
  label-language combinations; `test_metricOptions` checks the colour target
  combo in light and dark. Chinese uses 处理器, 内存, 显卡, 磁盘, 上行, 下行;
  English uses CPU, MEM, GPU, DISK, UP, DN.
- `test_iconModeMappingAndDisplay` opens with `labelMode: icon` and checks the
  exact six-glyph mapping, the combos' model and the Phosphor family on the
  preview frame.
- `test_labelModeChoiceEdits` selects Icons in the Text page and records one
  `labelMode` edit.

## Settings behaviour

- `advanced-settings.json` carries 43 advanced keys.
- `labelLanguage` accepts `follow`, `zh` and `en`, defaults to `follow`,
  rejects other values and round-trips through JSON.
- `labelMode` accepts `text` and `icon`, defaults to `text`, rejects other
  values and round-trips through JSON.
- Network rates use B/KBps/M/G units, carry at 1024, keep one decimal below 100,
  switch to integers at 100 and drop a trailing `.0`, so `1024.1 KBps` becomes
  `1 M`. Names are left-aligned and value+unit groups are right-aligned in each
  cell, with the gap absorbing length changes.
- `tiny` no longer overrides the cell height, so the height slider works in
  every layout; the plugin window's layout page applies scale to text, cells,
  padding and the glass frame together.
