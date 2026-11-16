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

Charts are P1; P0 uses tables and persists no chart history. Export appears only after SYS-F11 is implemented.

## Overlay

```text
┌ CPU 12%    RAM 8.1/32 GiB  ┐
│ LAN ↑ 1.2 MiB/s ↓ 4 MiB/s  │
└────────────────────────────┘
```

Candidate minimum font: 12 DIP in two compact rows. Names, units and unavailable states remain legible. No decorative charts by default. Configuration chooses metrics/adapters, font, background opacity, monitor, lock, click-through and optional fullscreen hiding. High contrast disables translucent backgrounds. Color alone never communicates state.

Unlocked overlays can be dragged; locked overlays cannot move. Click-through receives no mouse input, but sys → Edit overlay restores interaction. Right-click does not open a menu. Persist monitor identity and logical offset; missing monitors fall back to a visible primary work area. Never steal focus or cover Windows secure desktop.

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
