# System information test plan

[中文](TEST-PLAN.md) · [Requirements](plugin-design.en.md) · [UI](UI-SPEC.en.md)

2026-11-11. Core tests passed 1/1; hwinfo-backed Release build succeeded and isolated protocol query returned CPU/GPU/network fields, exited cleanly and wrote no stderr. Static hwinfo API smoke reported CPU, memory, OS, three GPUs, three disks, no battery and twelve interfaces. An installed-host isolated probe confirms plugin startup and about 64 MiB working set, but this hwinfo build has not yet been tested through the real launcher query. Earlier `sys ` crashes were diagnosed: Debug Qt imports caused `0xC0000135`; the 128 MB Job limit plus Qt Quick Controls recursion caused `0xC00000FD`; these were fixed with Release Qt, a 256 MB limit and plain QtQuick pages. UI/overlay visuals and hardware acceptance remain pending.

## Environment and evidence

Record Windows 10/11 versions, host/plugin commits and standard-user context. Include a desktop without a battery, a laptop, multiple adapters and mixed-DPI monitors. Document driver/VM capability differences. Use isolated settings without stopping user applications or replacing their configuration.

Raw evidence later goes in build/sysinfo-validation/; selected summaries in docs/validation/. Do not commit usernames, IP/MAC or raw machine diagnostics. Each record includes expected/actual/environment/evidence names. No screenshots or measurements currently exist.

## Cases

| ID | Requirement | Layer | Preconditions | Steps | Expected | Evidence | Status |
|---|---|---|---|---|---|---|---|
| TSYS-00 | SYS-F07/08/09 | L0 | Future package | Validate manifest, Qt dependencies, languages, permissions and absence of user data | Installable; permissions match use; no undeclared dependencies | package-audit | Manifest structure/permissions/Qt terms checked; full package audit not run |
| TSYS-01 | SYS-F01 | L3 | Real host enabled | Query sys; inspect summary and copy a metric | No query side effects; correct isolated copied value | host-query/copy | Typing `sys ` in an installed WinShun isolated profile expanded results without crash; isolated protocol returned P0 fields and four DXGI adapters with no QML bridge errors. Copy/details and settings visuals not tested |
| TSYS-02 | SYS-F02/12 | L2 | Fake healthy/unsupported/failed sources | Inject multiple adapters, absent battery and failed GPU | Distinct states; healthy metrics remain; missing is not zero | fake-matrix | NOT RUN |
| TSYS-03 | SYS-F03 | L2 | Fake clock/source | Open all three consumers; count source reads | Same snapshot agrees; no duplicate samplers | sample-count | NOT RUN |
| TSYS-04 | SYS-F04/09 | L4 | Unlocked desktop | Show/lock/click-through; edit through sys; switch foreground | Defaults off; editing remains recoverable; no focus stealing | overlay-video/screens | NOT RUN |
| TSYS-05 | SYS-F05/09 | L4 | Mixed-DPI monitors | Move/save/restart; unplug monitor; change scaling | Restore or safely clamp; retain preferred settings | monitor-matrix | NOT RUN |
| TSYS-06 | SYS-F06/12 | L1 | Fake monotonic time/counters | First sample, wrap, zero delta time, long resume interval | Correct baseline/stale state; no negative rates/spikes/divide-by-zero | delta-tests | Core tests pass for first sample, wrap, zero delta and stale state; sleep/resume case not run |
| TSYS-07 | SYS-F07/11 | L3 | Standard user; network/file observation | Open groups; export default summary; cancel second export | No automatic network/scanning/elevation; redacted export; cancel writes nothing | privacy-audit | NOT RUN |
| TSYS-08 | SYS-F08 | L3 | Sampling/overlay active | Disable/exit, including slow-source simulation | Workers/counters/windows released; no surviving process | process-before-after | Isolated protocol shutdown exits cleanly; real host disable/process-tree check not run |
| TSYS-09 | SYS-F09 | L4 | Themes/high contrast/languages/screen reader | Traverse keyboard; scale to 200%; use long values | Visible focus, no clipping/traps; no repeated live-value announcements | ui-matrix | NOT RUN |
| TSYS-10 | SYS-F10 | L4 | Windowed/borderless/exclusive applications | Toggle fullscreen and hiding setting | Hide/restore within documented coverage; retain preference | fullscreen-matrix | NOT RUN |
| TSYS-11 | SYS-F11 | L2 | Fake sensitive fields/write errors | Default/explicit export; cancel; reject writes | Redacted by default; explicit selection honored; failures preserve page | export-tests | NOT RUN |
| TSYS-12 | SYS-F06/12 | L4 | Two real adapters and sleep support | Transfer test data; disconnect; sleep/resume | Correct units/identity, baseline reset, no double counting | resume-network | NOT RUN |
| TSYS-13 | SYS-F02/03/06 | L5 | Reproducible idle/load | Compare disabled/enabled/three consumers/hidden | Measure CPU/memory/wakeup peaks; derive budget from peak plus margin | budget-report | NOT RUN |

## Layers and regression gates

L0 package checks; L1 pure core; L2 fake integration; L3 real host; L4 hardware interaction; L5 budgets/release. Compare GPU metrics with system tools using equivalent sampling semantics, not exact instantaneous equality. Any P0 failure, crash, misleading zero or unrecoverable click-through blocks release. Regress existing switches and host search entry points to confirm independence.

## Unverified

Temperature/fan/voltage are excluded from the first release. GPU coverage, fullscreen detection, shared Qt and budgets remain untested. Document link/ID auditing in this phase does not count as package L0 or product L3 acceptance.
