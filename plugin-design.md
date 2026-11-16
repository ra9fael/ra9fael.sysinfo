# 系统信息与桌面 overlay 设计

[English](plugin-design.en.md) · [UI](UI-SPEC.md) · [测试](TEST-PLAN.md)

日期：2026-11-11。状态：开发中；纯核心测试已执行，真实宿主、性能与真机交互仍未验证。

## 1. 关键裁决

一个采样服务产生不可变快照，搜索、详情与 overlay 共同消费；静态硬件信息、动态指标和可用能力分表存放。未知值显示“不可用”，不得用 0 伪装成功。独立于 winshun.switches，不修改系统状态。

## 2. 依据的事实

代码/协议阅读依据：[插件权限](../WinShun/docs/plugins.md)、[系统开关清单](../WinShun/src/switches-plugin/package/plugin.json)、[共享 Qt 协议](../WinShun/docs/plugins.md)。宿主有 ui、resident、共享 Qt 和外观通知机制；是否满足本插件全部交互仍需 L3。

用户明确要求独立 ra9fael.sysinfo、系统信息查询展示与桌面 overlay。PDH、DXGI、网络与电源 API 是候选数据源，不等于当前驱动支持所有指标。此前未经测量的 40–80 MB 数字不作为预算。

## 3. 架构与清单草案

纯核心负责采样差值、单位、时间戳、过期判断、布局状态；Windows 适配层负责 CPU/内存、磁盘、网卡、GPU、电源和静态系统信息。Qt UI 消费快照；协议线程不得等待慢速 WMI 查询。优先按事件刷新静态信息，动态采样按可见消费者合并调度。

清单文字草案：id=ra9fael.sysinfo；normal；resident；命令 sys；ui 权限；有 overlay 开启时常驻。eager/idleExitSec 策略需验证：启动时恢复已明确启用的 overlay 才需要 eager，默认关闭 overlay。共享 Qt 与最低宿主版本在 L3 探针后定，不创建可运行 manifest。

数据模型：MetricDefinition(id/unit/source/refreshClass)、MetricCapability(id/available/reason)、Sample(value/monotonicTime/status)、AdapterIdentity(instanceId/displayName)。身份不能用列表下标。候选数据源能力由表集中定义，所有消费者按表显示，不散落特判。

## 4. 核心逻辑与需求

P0 为首版必需；P1 为后续增强。每项必须按对应测试验收。

| ID | 优先级 | 需求及可观察验收 |
|---|---|---|
| SYS-F01 | P0 | sys 查询返回 CPU、内存与系统摘要；输入不启动外部命令，不修改系统；可复制独立指标。 |
| SYS-F02 | P0 | 详情展示 OS/CPU/内存/磁盘/网卡/GPU/电池分组；缺失字段明确标原因，支持多适配器。 |
| SYS-F03 | P0 | 搜索、详情、overlay 同一采样时刻数值一致；两个视图不启动两套采样器。 |
| SYS-F04 | P0 | overlay 默认关闭；显式开启、隐藏、锁定、穿透、退出都有可靠入口；穿透后有宿主命令可解除。 |
| SYS-F05 | P0 | 用户选择指标、网卡、单位、位置与显示器；重启恢复，显示器消失时窗口回可见工作区。 |
| SYS-F06 | P0 | 最小化/隐藏消费者后降低采样；休眠唤醒、计数器重置与网卡拔插后无负速率、峰值伪报或除零。 |
| SYS-F07 | P0 | 无管理员权限正常运行；不联网、不扫描用户文件、不采集温度；导出仅用户明确选择的内容。 |
| SYS-F08 | P0 | 停用/宿主退出后关闭窗口、取消工作并释放计数器和通知；不留后台进程。 |
| SYS-F09 | P0 | 明暗、缩放、中英、键盘与读屏可用；overlay 不抢输入焦点，详情允许正常键盘操作。 |
| SYS-F10 | P1 | 全屏时隐藏可选；检测范围与独占/无边框覆盖情况通过真机记录，不能承诺覆盖所有游戏。 |
| SYS-F11 | P1 | 导出诊断摘要，默认不含用户名、机器名、IP、MAC、序列号；取消导出不写磁盘。 |
| SYS-F12 | P0 | 快照带采样时间；超期标陈旧，错误与不支持分开。单个源失败不清空健康指标。 |

初次速率采样只有基线，第二次有效差值后显示数值。多网卡默认分开，汇总须明确选择，避免虚拟网卡重复计算。GPU 引擎计数不简单相加至超过 100%；其语义需源适配器说明与验证。

## 5. 交互

sys 打开摘要列表，详情动作用于打开独立面板；overlay 开关为显式动作，查询不改变可见性。稳定 ID 用 metric/category/adapterIdentity 组合，不由排序索引生成。空态、错误态与键盘线框见 UI spec。

## 6. 权限、数据与隐私

拟申请 ui；若通过宿主复制效果需要 clipboard:write，须核实当前契约并仅在确实需要时声明。normal 不代表管理员；任何 API 失败原样分类，不自动提权。设置保存选中指标和布局；不默认持久化采样历史。导出需用户确认目标目录，涉及 files:write 的清单变化须评审。

## 7. 失败矩阵

| 触发 | 表现与恢复 |
|---|---|
| PDH/驱动不支持字段 | 当前指标不可用，其他数据保留；不循环弹窗。 |
| 静态查询很慢 | 显示加载/超时状态；搜索仍响应；实现需可取消方案。 |
| 休眠/计数器回绕 | 丢弃跨异常时间差，重建基线。 |
| 显示器拔出/DPI 改变 | overlay 重新限制在工作区，不覆盖保存偏好直到用户移动。 |
| 全屏检测失败 | 用户快捷入口可隐藏；标明已测应用范围。 |
| 采样陈旧 | 标采样时间与陈旧状态；不把缓存当实时。 |
| 退出遇到阻塞源 | 取消/超时边界需 L3 验证，禁止 UI 线程等待不可控 API。 |

## 8. 交付清单

本阶段仅 README、设计、UI spec、测试计划中英版本。实施阶段再提供清单、源码、打包、双语帮助、精选验证证据。所有可见字串使用本地化资源；发布物不携带用户采样或导出内容。

## 9. 测试与预算

[测试计划](TEST-PLAN.md) 覆盖 L0–L5，L3 真实宿主与 L4 多显示器/休眠为首版门槛。候选采样间隔：可见动态数据 1 s、无可见消费者暂停动态采样；这是交互目标，需测 CPU/内存/唤醒次数后调整。先记录空载宿主与加载插件的差值，再用实测峰值乘余量决定 job 内存，不沿用脚手架数值。

## 10. 诚实笔记

已验证：纯核心单测、完整插件的独立 JSON-RPC query/QML/shutdown smoke test。独立查询可枚举 4 个 DXGI 图形适配器；仅显示名称/software 标记，不提供 GPU 利用率、显存、温度或风扇遥测。未验证：真实 Win顺 插件 runner、overlay 焦点/鼠标穿透、多 DPI/全屏、隐私监测、性能预算与宿主关闭进程树。运行中的正式 Win顺 实例未被改动。

## 11. 实施后回填

2026-11-11 实施回填：新增 Windows `GetSystemTimes` CPU 差值、`GlobalMemoryStatusEx` 内存、OS/CPU 型号、固定卷容量、`GetAdaptersAddresses`/`GetIfEntry2` 网络字节率、`GetSystemPowerStatus` 电池数据与 DXGI adapter inventory（不含 GPU telemetry）。共享采样快照每秒更新；速率以稳定适配器 identity 关联，计数器下降或首次样本不展示为有效速率。实现详情窗、基础 overlay 与锁定/穿透/显示位置偏好设置；manifest 声明 resident eager（用于恢复用户显式启用的 overlay）、共享 Qt 6.12、`ui` 权限和 sys 动作。核心单测通过；完整 Qt/QRC/MOC 插件可编译链接，独立协议 query 返回系统指标与 4 个 DXGI adapters，QML bridge errors 清零。安装版 WinShun 的隔离 profile 已实测：输入 `sys ` 后结果正常展开，无崩溃/重启。`0xC0000135` 定位为 Debug Qt DLL 依赖与宿主 Release Qt 不匹配；后续 `0xC00000FD` 由 manifest 的 128 MB Job 内存上限与 Qt Quick Controls 文本布局递归共同触发，现改为 256 MB 并使用纯 QtQuick 详情/设置页。宿主查询超时 3 秒，当前首次窗口加载约 1 秒。窗口、显示器/DPI/焦点穿透仍未验收；隐私监测与性能预算未测。
