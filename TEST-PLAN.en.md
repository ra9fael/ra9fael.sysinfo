# System information test plan

[中文](TEST-PLAN.md) · [Requirements](plugin-design.en.md) · [UI](UI-SPEC.en.md)

2026-11-21, version 1.6.0. Full build, plugin logic, independent rendering and dual-monitor integration pass. Installation reports enabled/running. Sampling cadence, four-edge click-through hover, font/material/chart visuals and short CPU/GPU probes have been measured. Complete product acceptance remains tracked below.

## Environment and evidence

Record Windows 10/11 versions, host/plugin commits and standard-user context. Include a desktop without a battery, a laptop, multiple adapters and mixed-DPI monitors. Document driver/VM capability differences. Use isolated settings without stopping user applications or replacing their configuration.

Selected results are in [desktop integration](docs/native-integration-results.md) and [material validation](docs/native-material-validation.md). Workspace screenshots are in `build/glass-reference-analysis/`. Record environment, expected and actual behavior; do not commit raw machine diagnostics.

## Native validation commands

From the plugin directory with Qt bin on PATH and an interactive Windows session:

```powershell
ctest --test-dir build --output-on-failure
./sysinfo.exe --verify-sampling
qmltestrunner -platform offscreen -input tests -o qml-junit.xml,junitxml
python tests/desktop_integration.py
python tests/desktop_integration.py --require-mixed-dpi
python tests/measure_overlay.py
```

`tests/tst_SettingsWindow.qml` loads the settings window from the source tree against a stub bridge, so it needs neither a build nor the host; the case list is in [layout control validation](docs/validation/layout-controls-test.md).

Python scripts retain development-workspace executable/Qt paths; adjust them on another machine. Performance measurement requires psutil. Strict mixed-DPI fails unless two native DPI values are observed. Partial coverage: TSYS-04 four-edge click-through wake/collapse; TSYS-05 dual-monitor movement/restart; TSYS-09 native font/theme/material snapshots; TSYS-13 short performance probes. None implies complete acceptance of that row.

## Cases

| ID | Requirement | Layer | Preconditions | Steps | Expected | Evidence | Status |
|---|---|---|---|---|---|---|---|
| TSYS-00 | SYS-F07/08/09 | L0 | Future package | Validate manifest, Qt dependencies, languages, permissions and absence of user data | Installable; permissions match use; no undeclared dependencies | package-audit | Manifest structure/permissions/Qt terms checked; full package audit not run |
| TSYS-01 | SYS-F01 | L3 | Real host enabled | Query sys; inspect summary and copy a metric | No query side effects; correct isolated copied value | host-query/copy | Typing `sys ` in an installed WinShun isolated profile expanded results without crash; isolated protocol returned P0 fields and four DXGI adapters with no QML bridge errors. Settings window checked by a standalone probe; copy/details not tested |
| TSYS-02 | SYS-F02/12 | L2 | Fake healthy/unsupported/failed sources | Inject multiple adapters, absent battery and failed GPU | Distinct states; healthy metrics remain; missing is not zero | fake-matrix | NOT RUN |
| TSYS-03 | SYS-F03 | L2 | Fake clock/source | Open all three consumers; count source reads | Same snapshot agrees; no duplicate samplers | sample-count | NOT RUN |
| TSYS-04 | SYS-F04/09 | L4 | Unlocked desktop | Show/lock/click-through; edit through sys; switch foreground | Defaults off; editing remains recoverable; no focus stealing | overlay-video/screens | Four-edge click-through expansion/collapse passes; full focus matrix pending |
| TSYS-05 | SYS-F05/09 | L4 | Mixed-DPI monitors | Move/save/restart; unplug monitor; change scaling | Restore or safely clamp; retain preferred settings | monitor-matrix | Dual-monitor movement/restart passes; native mixed-DPI and unplug pending |
| TSYS-06 | SYS-F06/12 | L1 | Fake monotonic time/counters | First sample, wrap, zero delta time, long resume interval | Correct baseline/stale state; no negative rates/spikes/divide-by-zero | delta-tests | Core tests pass for first sample, wrap, zero delta and stale state; sleep/resume case not run |
| TSYS-07 | SYS-F07/11 | L3 | Standard user; network/file observation | Open groups; export default summary; cancel second export | No automatic network/scanning/elevation; redacted export; cancel writes nothing | privacy-audit | NOT RUN |
| TSYS-08 | SYS-F08 | L3 | Sampling/overlay active | Disable/exit, including slow-source simulation | Workers/counters/windows released; no surviving process | process-before-after | Isolated protocol shutdown exits cleanly; real host disable/process-tree check not run |
| TSYS-09 | SYS-F09 | L4 | Themes/high contrast/languages/screen reader | Traverse keyboard; scale to 200%; use long values | Visible focus, no clipping/traps; no repeated live-value announcements | ui-matrix | Native theme/font/material snapshots checked; full accessibility pending |
| TSYS-10 | SYS-F10 | L4 | Windowed/borderless/exclusive applications | Toggle fullscreen and hiding setting | Hide/restore within documented coverage; retain preference | fullscreen-matrix | NOT RUN |
| TSYS-11 | SYS-F11 | L2 | Fake sensitive fields/write errors | Default/explicit export; cancel; reject writes | Redacted by default; explicit selection honored; failures preserve page | export-tests | NOT RUN |
| TSYS-12 | SYS-F06/12 | L4 | Two real adapters and sleep support | Transfer test data; disconnect; sleep/resume | Correct units/identity, baseline reset, no double counting | resume-network | NOT RUN |
| TSYS-13 | SYS-F02/03/06 | L5 | Reproducible idle/load | Compare disabled/enabled/three consumers/hidden | Measure CPU/memory/wakeup peaks; derive budget from peak plus margin | budget-report | Short native CPU/GPU/frame probes recorded; complete host budget pending |
| TSYS-14 | SYS-F09 | L4 | Monitor hidden or click-through | Open settings from the card and search, reopen, switch tabs and single settings | Reuse and focus the existing window; do not enable the monitor; changes apply immediately | settings-window | Standalone probe confirmed creation, visibility and foreground activation; real host card button pending |
| TSYS-15 | SYS-F07/09 | L1 | Older settings and plugin data directory | First-run migration, edit advanced values, restart, edit host values | Legacy values migrate once; advanced values persist; host values stay with the host | migration-tests | `sysinfo_tests` covers migration, persistence, host-key isolation and preview commit/cancel |
| TSYS-16 | SYS-F09 | L1 | Settings window from the source tree; stub bridge | Drag the interval slider and spin box, click each layout, switch metric language, then Apply and close directly | Only the release commits and the interval floors at 100 ms; layouts preview before Apply and closing drops the draft; short labels match the manifest in both languages | qml-tests | All 16 cases in `tests/tst_SettingsWindow.qml` pass |

## Layers and regression gates

L0 package checks; L1 pure core; L2 fake integration; L3 real host; L4 hardware interaction; L5 budgets/release. Compare GPU metrics with system tools using equivalent sampling semantics, not exact instantaneous equality. Any P0 failure, crash, misleading zero or unrecoverable click-through blocks release. Regress existing switches and host search entry points to confirm independence.

## Unverified

Cross-device GPU coverage, native mixed-DPI, external recording, full accessibility and complete host resource budgets remain outstanding. Export and automatic fullscreen hiding are not implemented.
