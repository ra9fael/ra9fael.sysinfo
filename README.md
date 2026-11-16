# ra9fael.sysinfo

[English](README.en.md)

状态：开发中，2026-11-11。新增 HTML 界面原型 `design/sysinfo-mockup.html`，双击即可预览明暗主题、硬件概览和 overlay 预览。采集实现改用 vendored hwinfo 2 API（CPU、内存、OS、GPU、磁盘、电池、网卡信息）；hwinfo 目前不提供网络吞吐率，所以仅为适配器收发计数保留 Win32 IP Helper 适配。CMake Release 构建和核心单测通过，独立协议查询返回 CPU、内存、磁盘、网络及 3 个 GPU。

此前崩溃也已修复：Debug 插件依赖宿主没有的 Qt Debug DLL，导致 `0xC0000135`；输入 `sys ` 时的 `0xC00000FD` 来自 128 MB Job 内存上限与 Qt Quick Controls 文本布局递归。当前 manifest 使用 256 MB，详情/设置页改为纯 QtQuick；上一次 Release 包在安装版 WinShun 隔离 profile 中查询成功。本轮 hwinfo 版本尚待安装版宿主复验。

hwinfo 以 Git submodule 接入：[`lfreist/hwinfo`](https://github.com/lfreist/hwinfo)，版本 `2.0.0`，固定提交 `acdc2eb4a9109b4ea6bbe207e03d9da33bc16fab`，MIT 许可；上游源码未修改，许可证位于 `third_party/hwinfo/LICENSE`。本环境需配置 `http://127.0.0.1:7897` 访问 GitHub。

系统信息查询、详情面板与桌面 overlay 共用采样快照。采集不联网、不启动外部进程、不扫描用户文件、不提权、不修改系统设置。GPU 名称/规格与遥测仅展示 hwinfo/驱动实际返回的可选字段；不保证每个 GPU 都提供利用率、显存或温度。插件独立于系统开关插件。

- [功能与设计](plugin-design.md)：范围、需求 ID、架构、权限、失败矩阵。
- [UI 规格](UI-SPEC.md)：布局、字段、交互与状态。
- [测试计划](TEST-PLAN.md)：L0–L5 与需求对应的验收用例。

温度、风扇、电压等字段取决于硬件 API 能力；导出未实现。代码与原型检查不代表产品验收。
