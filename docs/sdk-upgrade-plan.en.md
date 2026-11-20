# sysinfo: settings and search integration plan

[中文](sdk-upgrade-plan.md)

## Why change

The manifest exposes more than forty settings on one host card. Layout strings, font names, comma-separated colors and many material parameters are hard to understand or tune in that form. A long card also makes simple operations such as disabling click-through harder to find.

The plugin already sends setting changes to the host, so synchronization should be retained rather than rebuilt. Its current UI bridge exposes the native overlay but no dedicated settings window. A settings hint already refers to recovering click-through from a settings window; that entry must actually exist.

`main.cpp::queryResult()` reads the base result object with `toArray()`, then replaces `items` with that result. This can discard the search rows and must be corrected when updating search behavior.

## User-facing scheme

The search list shows the monitor state, live metric values and a settings entry. The monitor row offers show/hide, lock/unlock, click-through and edge-hide actions. Metric rows retain copying the metric value. Action titles reflect current state.

The host card keeps these simple settings:

- Monitor visibility and sampling interval.
- Tiny mode and overall scale.
- Lock position, edge auto-hide and click-through.

The dedicated settings window contains these groups:

| Group | Contents | Editing method |
|---|---|---|
| Layout | Metric order, rows, columns, blank cells, cell sizes, padding and dividers | Cell selection, layout preview and numeric controls |
| Text | Font, text size, labels, values and units | Font picker, size input and switches |
| Charts | Shape, placement, height, dots, colors and opacity | Choices, color pickers and sliders |
| Material | Theme, material, transparency, blur, refraction, dispersion, rim light, tile size and radius | Choices and sliders with exact inputs |
| Position and interaction | Display target, position preset, snapping, animations and material motion | Choices and switches |
| Capture | Visibility in screenshots and recording | Switch with an explanation |

The window also exposes common host-owned settings so it can configure the monitor in one place. Those controls use host synchronization. Advanced controls use the plugin settings store.

The currently inactive `shadow` parameter must not appear as a working control. Preserve any stored value without suggesting that the renderer supports it.

## Implementation

1. Update the vendored SDK and rename `updateHostSettings()` calls to `updateSettings()`.
2. Correct result wrapping by retaining the base `items` member and adding the desired rerun interval.
3. Add `settingsWindow: true`, implement `showSettings()`, and route the search settings action to the same UI-thread entry. Reopening focuses the existing window.
4. Extend the existing settings store with persistence for advanced parameters. Keep host-owned settings and window position separate from that advanced-settings file.
5. Transfer existing advanced values before removing their manifest declarations. Verify whether the host still exposes persisted values after a key is removed. If it does not, read and transfer the affected keys while they remain declared. Never silently replace existing values with defaults.
6. Route overlay edits, search actions and settings controls through the same patch-and-apply path. Send only changed host-owned keys to the host.
7. Make visual adjustments preview on the overlay. Commit numeric drags at release rather than writing every pointer event; individual control changes are immediate and require no whole-window Save. Store layout drafts until a valid layout is committed.
8. Apply host appearance to the settings window, including language, colors and radii. Keep the overlay's independently selected theme semantics.

Sampling uses the existing 100–5000 ms range; scale uses 80–200%. Other ranges come from current settings validation. Preserve integer precision unless the current parameter already supports fractions. Special values such as automatic chart height need an explicit automatic option rather than a misleading continuous range.

## Acceptance

- Search shows monitor and metric rows correctly and periodically refreshes them.
- Search, host card and window agree after changing every common setting and after restart.
- Existing advanced values survive the ownership change, including colors and metric layouts.
- Changing a material or layout control produces the corresponding preview and persists the committed value.
- The settings window remains usable when the overlay is locked, hidden or click-through; restoring pointer interaction is always accessible.
- Opening settings does not enable the overlay or start a scan-like operation.
- Relevant settings and search protocol tests pass; UI verification covers Chinese/English, light/dark and common DPI scales.

## Documentation

Update README and UI documentation with the simple/advanced split, both settings entry points, layout/color editing and click-through recovery. Describe only material controls that affect the current renderer.
