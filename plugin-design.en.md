# System information and desktop overlay design

[中文](plugin-design.md) · [UI](UI-SPEC.en.md) · [Tests](TEST-PLAN.en.md)

Date: 2026-11-09. Proposal only; implementation, performance and hardware interaction are unverified.

## 1. Verdict

One sampler publishes immutable snapshots shared by search, details and overlay. Static information, dynamic metrics and capabilities have separate tables. Unknown values are unavailable, never fabricated zeroes. The plugin is independent of winshun.switches and does not change system state.

## 2. Grounded facts

Code/protocol sources: [permissions](../WinShun/docs/plugins.md), [switches manifest](../WinShun/src/switches-plugin/package/plugin.json), [shared Qt protocol](../WinShun/docs/plugins.md). The host provides ui, resident execution, shared Qt and appearance notifications. Full suitability requires L3 verification.

The user requested ra9fael.sysinfo, system queries and an overlay. PDH, DXGI, network and power APIs are candidates, not guarantees of driver support. The earlier unmeasured 40–80 MB estimate is not a budget.

## 3. Architecture and manifest proposal

The pure core handles deltas, units, timestamps, stale state and layout. Windows adapters provide CPU/memory, disk, network, GPU, power and static information. Qt consumes snapshots. The protocol thread never waits for slow WMI work. Refresh static information on events where possible; coalesce dynamic sampling across visible consumers.

Manifest proposal in prose: id ra9fael.sysinfo; normal; resident; sys command; ui permission; remain resident while an overlay is enabled. Verify eager/idleExitSec policy: eager is needed to restore an explicitly enabled overlay; overlay defaults off. Shared Qt and minimum host version require L3. No runnable manifest is created.

Models: MetricDefinition(id/unit/source/refreshClass), MetricCapability(id/available/reason), Sample(value/monotonicTime/status), AdapterIdentity(instanceId/displayName). Identity cannot be a list index. Central capability tables drive all views.

## 4. Core and requirements

P0 is required for the first release; P1 is subsequent enhancement.

| ID | Priority | Requirement and observable acceptance |
|---|---|---|
| SYS-F01 | P0 | sys returns CPU, memory and system summaries; queries launch no external command or state mutation; individual metrics can be copied. |
| SYS-F02 | P0 | Details group OS/CPU/memory/disk/network/GPU/battery; missing fields state why; multiple adapters are supported. |
| SYS-F03 | P0 | Search, details and overlay agree for a sampling timestamp; multiple views do not create duplicate samplers. |
| SYS-F04 | P0 | Overlay defaults off; explicit show/hide/lock/click-through/exit actions remain reachable, including a host command to undo click-through. |
| SYS-F05 | P0 | Metrics, adapters, units, location and monitor are configurable and restored; disconnected monitors cannot strand the overlay. |
| SYS-F06 | P0 | Hidden consumers reduce sampling; resume, counter resets and adapter removal produce no negative rate, fake spike or divide-by-zero. |
| SYS-F07 | P0 | Runs without elevation; no network, user-file scanning or temperature collection; exports include only explicit selections. |
| SYS-F08 | P0 | Disable/host exit closes windows and cancels workers/counters/notifications without surviving processes. |
| SYS-F09 | P0 | Light/dark, scaling, Chinese/English, keyboard and screen readers work; overlay does not steal focus; details support keyboard input. |
| SYS-F10 | P1 | Optional fullscreen hiding documents tested exclusive/borderless coverage; no universal game compatibility claim. |
| SYS-F11 | P1 | Diagnostic export omits username, hostname, IP, MAC and serial by default; cancelling writes nothing. |
| SYS-F12 | P0 | Snapshots have timestamps; stale, error and unsupported states differ; one source failure preserves healthy data. |

The first rate sample establishes a baseline; only the next valid delta yields a rate. Show adapters separately by default; explicit aggregation must avoid virtual-adapter double counting. GPU engine values must not simply sum beyond 100%; adapters document and verify semantics.

## 5. Interaction

sys shows summary rows; an explicit action opens details. Overlay toggling is an action, never a query side effect. Stable IDs combine metric/category/adapter identity, not sort position. See UI spec for wireframes, empty/error states and keyboard rules.

## 6. Permissions, data and privacy

Proposed permission: ui. Verify whether host copy effects require clipboard:write and declare it only if needed. Normal execution does not imply elevation; classify API failures without automatic elevation. Persist selections/layout, not sample history by default. Exports require an explicit destination; review any files:write declaration change.

## 7. Failure matrix

| Trigger | Visible behavior and recovery |
|---|---|
| Unsupported PDH/driver field | Mark only that metric unavailable; retain others; no repeated dialogs. |
| Slow static query | Show loading/timeout; search stays responsive; cancellation requires implementation validation. |
| Resume/counter wrap | Discard invalid deltas and rebuild baseline. |
| Monitor removal/DPI change | Clamp overlay to work area without overwriting preferred location until user movement. |
| Fullscreen detection failure | User can hide through a reliable command; document tested applications. |
| Stale samples | Show timestamp/stale status rather than claiming live data. |
| Blocking source on shutdown | Verify cancellation/time boundaries at L3; never block UI on an uncontrolled API. |

## 8. Deliverables

This phase produces bilingual README, design, UI and test plans only. Implementation later adds manifest, sources, packaging, bilingual help and selected evidence. Localize visible strings; never package user samples or exports.

## 9. Tests and budgets

[Test plan](TEST-PLAN.en.md) covers L0–L5; real-host L3 and multi-monitor/resume L4 gate the first release. Candidate cadence: visible dynamics at 1 s, dynamic sampling paused without visible consumers. These are interaction targets, adjusted after measuring CPU/memory/wakeups. Measure the difference from the idle host, then set job memory from peak use plus documented margin, not scaffold values.

## 10. Honest notes

Verified: pure-core unit tests and a complete plugin's isolated JSON-RPC query/QML/shutdown smoke test. The query enumerated four DXGI adapters; GPU data is limited to names/software flags, not utilization, memory, temperature or fan telemetry. Not verified: overlay focus/click-through, mixed DPI/fullscreen, privacy instrumentation, performance budget and real host shutdown. The installed WinShun instance was not modified.

## 11. Implementation addendum

2026-11-11 implementation addendum: Windows `GetSystemTimes` CPU deltas, `GlobalMemoryStatusEx` memory, OS/CPU names, fixed-volume capacity, `GetAdaptersAddresses`/`GetIfEntry2` network byte rates, `GetSystemPowerStatus` battery data and DXGI adapter-name inventory (no GPU telemetry). One shared snapshot updates once per second; network rates are keyed by stable adapter identity and first/reset samples are not displayed as valid rates. Details, baseline overlay, lock/click-through and position persistence are implemented. The manifest is resident/eager (to restore an explicitly enabled overlay), uses shared Qt 6.12 and the `ui` permission. Core tests pass; the full Qt/QRC/MOC plugin builds and links; isolated protocol query returns system data and four adapters with no QML bridge errors. In an installed WinShun isolated profile, typing `sys ` now returns results without crash or restart. Import-table inspection identified Debug Qt DLL dependencies as the cause of `0xC0000135`; QRC was unrelated. The later `0xC00000FD` was caused by the manifest's 128 MB Job memory limit combined with recursion in Qt Quick Controls text layout; it is now 256 MB and the details/settings pages use plain QtQuick. First window load is about one second against the host's three-second query timeout. Display, focus/click-through and full shutdown acceptance remain pending; privacy instrumentation and performance budgets have not been measured.
