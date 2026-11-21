# Minimum presets validation

Date: 2026-11-21

## Behavior

- Every preset uses 80×28 logical-pixel cells, zero padding and 80% scale.
  Compact bar, standard grid and tall stack use 6, 3 and 1 columns.
- Preset card miniatures have a fixed layout slot independent of their grid's
  implicit width. Repeated settings echoes do not reload unchanged values.
- Closed metric selectors and popup delegates render Phosphor glyphs. Switching
  label modes leaves the six metric IDs and their order unchanged.
- Settings and monitor windows default to WinShun's bundled Alibaba PuHuiTi 3.0.
- Rates use B/s, KB/s, MB/s, GB/s and TB/s, scale by 1024 and retain one decimal
  in every range. Units remain smaller when long readings shrink to fit.

## Automated results

- Release build: MSVC x64, Qt 6.12.0.
- CTest logic suite: 1 passed, 0 failed.
- QML settings suite: 23 passed, 0 failed.
- `test_repeatedPresetsWithBackendFeedback`: 60 preset selections with synchronous
  settings normalization and feedback; checks selection, dimensions, controls
  and cell order after each selection.
- `test_toggleIconModeKeepsOrder`: 12 text/icon switches; checks metric order,
  selected label font and default settings-window family.
- Logic assertions cover rate decimals and unit boundaries from B/s to TB/s.

## Native window results

An isolated `--probe --probe-settings` process loads fonts from
`C:/Program Files/WinShun`. Windows UI Automation invokes each of the three
preset buttons ten times and verifies its selected accessible name after every
invocation: 30 selections, all correct; the process remains responsive.

Screenshot: `%LOCALAPPDATA%/Temp/opencode/sysinfo-layout-fixes/settings.png`.
The closed selectors display CPU, memory, GPU, disk, upload and download icons;
the tall-stack preset is highlighted and reports 64×134 physical pixels.

## Installation

Package: `dist/manual-test-20261121-minimum-presets/ra9fael.sysinfo-1.6.0.wsplugin`,
1,010,173 bytes. Installed files match the staged files byte for byte. Saved
plugin data files are unchanged. The previous executable remains as
`sysinfo.exe.old-20261121-174242`; the host launched the installed executable
at 17:42:44.
