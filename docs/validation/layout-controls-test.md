# Layout controls validation

## Package

`dist/manual-test-20261121-0843/ra9fael.sysinfo-1.6.0.wsplugin`, 881013
bytes, containing `plugin.json`, the two READMEs and `sysinfo.exe`.

Installed over the running copy by replacing `plugin.json`, the two READMEs and
`sysinfo.exe` in `%LOCALAPPDATA%\WinShun\plugins\ra9fael.sysinfo`. Superseded
executables are renamed to `sysinfo.exe.old-<timestamp>` instead of deleted, so
a running plugin process keeps its image. `WinShun.exe` was not restarted.
`plugins/ra9fael.sysinfo.data/advanced-settings.json` and `window.json` were
not modified by this install, so the saved position, material, font and metric
choices survived. The four installed files hash-match the staged package.

## Build

- `sysinfo.exe` and `sysinfo_tests` built in Release with Qt 6.12.0 MSVC 2022 x64.
- `sysinfo_core` passed; `sysinfo tests: all passed`.
- `qmllint SettingsWindow.qml` reports no errors and no warnings beyond the
  file's pre-existing unqualified-access ones.

## Probe run

The installed executable was also launched standalone against a scratch data
directory, so the live plugin data was untouched:

```
sysinfo.exe --probe --probe-data <tmp>\data --light --settings-json <overrides.json> --dump-native <tmp>\overlay.png
```

It exited 0 and wrote the overlay snapshot. With `"labelLanguage": "zh"` and
three columns the rows read 处理器 磁盘 上行 and 内存 显卡 下行; with `"en"` they
read CPU DISK UP and MEM GPU DOWN, each followed by its value and unit.
Snapshots: `%TEMP%\opencode\probe-20261121\overlay.png` and `overlay-en.png`.

## Host font

The font family remains editable. A fresh `Settings` and a JSON with an empty
`fontFamily` both resolve to `Alibaba PuHuiTi 3.0`; a saved family is preserved.
The plugin loads `<host app>/fonts/AlibabaPuHuiTi-3-55-Regular.ttf` into Qt and,
because the native overlay draws with DirectWrite, builds a font collection from
the same file. The host app folder comes from the parent process image, since
plugins are installed outside it.

For a rendering check the probe takes `--probe-app-dir C:\Program Files\WinShun`:

```
sysinfo.exe --probe --probe-data <tmp>\data --light --settings-json <overrides.json> --probe-app-dir <host app> --dump-native <tmp>\with-collection.png
```

Over the 248×64 overlay, the snapshot with the host font differs from the same
run without it in 7295 pixels, while a run before the change differed in 14
pixels (only the changing sample digits). A `fontFamily: Consolas` override
changes 2532 pixels, so the measurement detects a real font change. The Chinese
snapshot reads 处理器 磁盘 上行 / 内存 显卡 下行 in PuHuiTi. Snapshots:
`%TEMP%\opencode\font-probe-20261121\with-collection.png` and `zh-puhuiti.png`.

## Settings window suite

`tests/tst_SettingsWindow.qml` drives the real `SettingsWindow.qml` against a
stub bridge that records edits, previews and layout applies. It runs headless:

```
qmltestrunner -platform offscreen -input <repo>/tests -o <log>,junitxml
```

15 test functions, 16 cases, 0 failures.

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

- `test_layoutRowPreviewsWithoutSaving` types a cell width on the layout page:
  one preview, no edit and no layout apply, and the preview carries only
  `cellWidth`.
- `test_presetPreviewsWithoutSaving` clicks the compact bar preset: one
  preview, no edit, no apply.
- `test_restorePreviewsStandardGrid` clicks restore: the preview drafts the
  standard grid with scale back at 100.
- `test_applySendsWholeDraft` clicks Apply layout: one `applyLayout` carrying
  metric order, columns, cell width and height, padding, scale and tiny
  together, so the store commits a single revision.
- `test_closeDropsDraft` closes the window: one `cancelPreview`, and the
  window hides instead of closing so the host can reopen it.

### Metric labels

`test_metricLabels` runs the cell-order combos under all four locale and
label-language combinations, and `test_metricOptions` checks the colour target
combo in light and dark:

- Chinese uses 处理器, 内存, 显卡, 磁盘, 上行, 下行; English uses CPU, MEM,
  GPU, DISK, UP, DOWN.
- The blank slot follows the label language rather than the window locale, so
  it never mixes languages with the metric entries, and the trailing-blank
  trim in the layout preview compares against the same string the combo
  shows.

## Settings behaviour

- `advanced-settings.json` carries 42 advanced keys.
- `labelLanguage` accepts `follow`, `zh` and `en`, defaults to `follow`,
  rejects other values and round-trips through JSON.
- `tiny` no longer overrides the cell height, so the height slider works in
  every layout; the plugin window's layout page applies scale to text, cells,
  padding and the glass frame together.

## Pending

- Rendering evidence for the new layout page in an installed host: preset
  selection, the actual size readout and the label language switch still need a
  screenshot from a real desktop session.
