# ra9fael.sysinfo

[中文](README.md)

Status: in development, 2026-11-11. The HTML UI prototype at `design/sysinfo-mockup.html` previews light/dark themes, the hardware overview and desktop overlay. Collection now uses vendored hwinfo 2 APIs for CPU, RAM, OS, GPU, disks, batteries and network interfaces. hwinfo does not currently expose network throughput, so a small Win32 IP Helper adapter remains only for interface byte counters. The Release CMake build and core tests pass; isolated protocol query returns CPU, memory, disk, network and three GPUs.

Earlier crashes are fixed: Debug Qt imports unavailable in the installed host caused `0xC0000135`; the `0xC00000FD` on `sys ` came from the 128 MB Job limit combined with recursive Qt Quick Controls text layout. The manifest now allows 256 MB and details/settings use plain QtQuick. The previous Release binary passed a `sys ` query in an installed WinShun isolated profile. This hwinfo-backed build still needs installed-host verification.

hwinfo source: [`lfreist/hwinfo`](https://github.com/lfreist/hwinfo), version `2.0.0`, snapshot `acdc2eb4a9109b4ea6bbe207e03d9da33bc16fab`, MIT license; the upstream license is retained at `third_party/hwinfo/LICENSE`. hwinfo is integrated as a Git submodule from [`lfreist/hwinfo`](https://github.com/lfreist/hwinfo), version `2.0.0`, pinned to `acdc2eb4a9109b4ea6bbe207e03d9da33bc16fab`, under MIT license. Upstream source is unmodified; its license is at `third_party/hwinfo/LICENSE`. This environment requires the `http://127.0.0.1:7897` proxy for GitHub.

Queries, details and the desktop overlay share a sampler snapshot. Collection does not access the network, launch external processes, scan user files, elevate or change system settings. GPU data is limited to fields hwinfo/drivers actually provide; utilization, memory and temperature are not guaranteed on every adapter. This plugin is independent of System switches.

- [Functional design](plugin-design.en.md): scope, requirement IDs, architecture, permissions and failure matrix.
- [UI specification](UI-SPEC.en.md): layout, fields, interaction and states.
- [Test plan](TEST-PLAN.en.md): L0–L5 acceptance cases traced to requirements.

Temperature, fan speed, voltage and other sensor data depend on hardware API support; export is not implemented. Code and prototype checks do not constitute product acceptance.
