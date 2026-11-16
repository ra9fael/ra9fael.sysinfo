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

`tiny` uses single-line text, fixed 28px rows, 4px outer padding and a 10px radius
cap, all following overall scale. Text shrinks to its available area. Cell width,
blank cells and label/value/unit switches are preserved. `inline/dotted/bars`
occupy a separate 6px bottom strip in tiny mode; `wash` sits behind text without
reserving extra height.

```text
┌ CPU 12%    RAM 8.1/32 GiB  ┐
│ LAN ↑ 1.2 MiB/s ↓ 4 MiB/s  │
└────────────────────────────┘
```

Normal and compact native grids honor explicit rows and columns with centered text. `cpu,memory,gpu;disk,netUp,netDown` creates two rows and three columns; empty cells are allowed. Family, 70–180% font size and cell dimensions are configurable. Measured long text shrinks to fit. Labels, values and units have separate switches.

Default `inline` draws a line below the text; `wash` fills a translucent background area; `dotted` draws sample points; `bars` draws narrow columns; `off` hides charts. `waveColors` accepts one shared color or six colors in CPU, memory, GPU, disk, upload, download order. History retains 60 samples at the configured 500–5000 ms metric interval.

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

Details Tab order: category → metrics → actions → overlay toggle. Arrows change categories/rows; Enter selects; Escape closes details without closing an explicitly enabled overlay. Disabling the plugin closes all windows. Settings focus enters the first field and returns to its trigger on close.

Provide accessible icon names. Do not announce values every second; announce error transitions once. Tables support keyboard copy without overriding search shortcuts. Candidate target sizes: at least 28 DIP, 32 DIP for frequent toggles, adjusted to host scaling. Visually verify 100/125/150/200%, mixed DPI and long English/Chinese strings. Inherit host theme/type/radii and semantic color tokens rather than inventing an unverified palette.

## UI acceptance

SYS-F09 maps to TSYS-09: no clipped primary action, focus trap or missing accessible name. TSYS-04/05 cover click-through recovery and monitor changes; TSYS-08 covers shutdown. Without rendered evidence, UI cannot be marked passed.
