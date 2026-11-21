# System information UI specification

[中文](UI-SPEC.md) · [Design](plugin-design.en.md) · [Tests](TEST-PLAN.en.md)

Proposal and implementation are maintained together. A `sys` summary query, details window, desktop overlay, and basic settings are implemented. A full category/table layout, export, fullscreen hiding, complete keyboard/screen-reader accessibility, and multi-monitor DPI behavior remain unimplemented or unverified. Covers SYS-F01–SYS-F12.

## Pages and entry points

sys summary rows order CPU, memory, network, disk and more information. Titles contain metric/value; subtitles contain units, adapter and sample time. Enter opens category details. Copy and overlay operations have explicitly named secondary actions. Missing data cannot produce copyable fake zeroes.

Details candidate size: 960×640 DIP initially, minimum 640×480. These are layout targets requiring L4. At narrow widths, replace the sidebar with a category picker; tables scroll horizontally without clipping actions.

```text
┌ System information                  [Export] [Settings] ┐
│ CPU      │ CPU · model                         Live     │
│ Memory   │ Usage 12% · frequency if available           │
│ Disk     │ [Time/value chart or unavailable reason]     │
│ Network  │ Metric            Value           Status      │
│ GPU      │ …                                             │
│ Battery  │ Source / timestamp                            │
├──────────┴───────────────────────────────────────────────┤
│ [Desktop overlay: Off]       Last sample / stale notice   │
└──────────────────────────────────────────────────────────┘
```

Detail charts remain planned. Desktop charts use 60 in-memory samples without persistence. Export appears only after SYS-F11 is implemented.

## Overlay

Normal rows default to 40px with 4px `padding`.
Column `dividers` default to enabled at 12% `dividerOpacity`. Filled dots use a
3px grid; bars aggregate history to available width while retaining peaks.
Settings are split: the host card keeps General (show, sampling interval, Tiny,
scale, lock, edge auto-hide, click-through); the plugin window holds General,
Layout, Text, Charts, Material and Interaction & capture. The Layout page holds
columns, cell width, cell height and padding, plus three presets (compact bar,
standard grid, tall stack) that combine those values with a metric arrangement.
Clicking a preset previews the full layout immediately — cell controls, metric
arrangement, preview frame and a persistent selected state; size edits and
Restore default stay drafts too, and only Apply layout saves. Closing the
window cancels the draft. Scale applies to text, cells, padding and the glass
frame together, and the page reports the resulting pixel size. Value sliders
follow the pointer while dragged and commit once on release. The Text page
exposes metric label language (`follow`/`zh`/`en`), using the short names CPU,
MEM, GPU, DISK, UP and DN or 处理器, 内存, 显卡, 磁盘, 上行 and 下行 so mixed CJK
and Latin glyph heights can be levelled. A separate label mode switches those
names for Phosphor glyphs (CPU/memory/graphics-card/hard-drives/arrow-up/
arrow-down) while values and units remain; the embedded regular face keeps the
icon mapping identical in the settings window and the native overlay. Network
rates drop the redundant `/s`, carry at 1024, keep one decimal below 100 and
switch to integers at 100.
`edgeSnap` snaps within 12px on drag release;
`edgeHide` independently controls collapse and hover expansion. Glass
transparency controls backdrop/tint mixing without changing content opacity.

`tiny` compacts text only: 13px type, a 10px radius cap and units kept visible
when values are hidden. Row height, padding and the glass frame follow
`cellHeight`, `padding` and overall scale. Text shrinks to its available area. Cell width,
blank cells and label/value/unit switches are preserved. Every shape supports
below-text or background placement; background charts reserve no extra height.

```text
┌ CPU 12%    RAM 8.1/32 GiB  ┐
│ UP 1.2M       DN 4M        │
└────────────────────────────┘
```

The native grid honors explicit rows and columns with centered text. `cpu,memory,gpu;disk,netUp,netDown` creates two rows and three columns; empty cells are allowed. Family, 70–180% font size and cell dimensions are configurable. The default family is the host's Alibaba PuHuiTi 3.0 from its `fonts` folder, matching WinShun's own UI; any other family uses the system fonts. Measured long text shrinks to fit. Labels, values and units have separate switches.

Shapes are `inline` lines, `wash` areas, `dotted` dots, `bars` columns and `off`.
`wavePlacement` selects `below`, `background` or `auto` (wash behind text, others
below). With charts enabled, text is a centered single line. Below-text layouts
place text at the top in a 16px box for 28px rows, or 18px otherwise. Top and
bottom insets are 2px, with a 2px automatic text/chart gap: 28/36/40/56px rows
allocate 6/12/16/32px charts. `waveHeight=0` is automatic; custom 4–136px heights
are capped to available space. Smaller custom charts leave more space between
text and charts; setting 0 restores automatic height. Background charts use row
height minus 4px.

`waveDots` selects `filled` baseline fill or `trace` outline dots. `waveOpacity`
controls background opacity, default 55%; `waveBelowOpacity` defaults to 95%.
Both accept 0–100%. `waveColors` accepts one shared color or six colors in
CPU, memory, GPU, disk, upload, download order. Percentages use a fixed 0–100%
axis; network rates scale to history peaks. History retains 60 samples at the
configured 100–5000 ms metric interval.

`system/light/dark` controls text and glass tint. `liquid/frosted/square` selects liquid, frosted or square-embossed glass with 8–80 pixel tiles. The animation setting controls local hover refraction, eased pressing, drag flow and edge morphing.

Unlocked overlays can be dragged; locked overlays cannot move. Click-through uses screen-pointer detection to expand the half-circle on hover and collapse after pointer exit. Right click opens no menu. Physical coordinates and monitor name persist across restarts. The overlay does not steal focus. High contrast and full keyboard/screen-reader acceptance remain outstanding; automatic fullscreen hiding is not implemented.

## Fields and states

- CPU/memory: percentage and used/total; unavailable values show a dash and reason.
- Network: per-adapter send/receive rates; distinguish link speed from traffic; show disconnection.
- Disk capacity differs from throughput; GPU memory differs from system memory.
- Battery: no battery means not applicable; distinguish charging, power source and estimated remaining time.
- State machine: loading → fresh / unsupported / error; fresh → stale; recovery returns to fresh. Initial rate baseline shows Sampling.
- Source failures are nonmodal; export failures retain page and selections.

## Keyboard, accessibility and appearance

Details Tab order: category → metrics → actions → overlay toggle. Arrows change categories/rows; Enter selects; Escape closes details without closing an explicitly enabled overlay. Disabling the plugin closes all windows. The settings window is independent of
the card; opening it takes foreground focus and closing it returns to the
previous foreground window.

Provide accessible icon names. Do not announce values every second; announce error transitions once. Tables support keyboard copy without overriding search shortcuts. Candidate target sizes: at least 28 DIP, 32 DIP for frequent toggles, adjusted to host scaling. Visually verify 100/125/150/200%, mixed DPI and long English/Chinese strings. Inherit host theme/type/radii and semantic color tokens rather than inventing an unverified palette.

## UI acceptance

SYS-F09 maps to TSYS-09: no clipped primary action, focus trap or missing accessible name. TSYS-04/05 cover click-through recovery and monitor changes; TSYS-08 covers shutdown. Without rendered evidence, UI cannot be marked passed.
