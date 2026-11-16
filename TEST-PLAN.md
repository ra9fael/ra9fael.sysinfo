# 系统信息测试计划

[English](TEST-PLAN.en.md) · [需求](plugin-design.md) · [UI](UI-SPEC.md)

2026-11-16，1.4.0-dev。完整构建、插件逻辑、独立渲染和双屏桌面集成通过，安装回读为启用且运行中。已测采样周期、四边穿透 hover、字体/材质/波形与短时 CPU/GPU 成本。完整产品验收仍按下表逐项跟踪。

## 环境与证据

Windows 10/11 标记版本、宿主/插件提交、普通用户；至少一台无电池设备、一台笔记本、多网卡与混合 DPI 多显示器。驱动/虚拟机能力差异单列。准备独立测试配置，不停止用户程序、不替换用户设置。

精选结果见 [桌面集成](docs/native-integration-results.md)与[材质验证](docs/native-material-validation.md)。工作区截图位于 `build/glass-reference-analysis/`。证据记录环境、预期和实际结果，不提交原始整机诊断。

## 原生验证入口

从插件源码目录运行，Qt bin 在 PATH 中；桌面测试需要交互式 Windows 会话：

```powershell
ctest --test-dir build --output-on-failure
./sysinfo.exe --verify-sampling
python tests/desktop_integration.py
python tests/desktop_integration.py --require-mixed-dpi
python tests/measure_overlay.py
```

Python 脚本沿用开发工作区的可执行文件/Qt 路径，换机器需调整。性能脚本依赖 psutil。严格 mixed-DPI 用例在未观测到两种原生 DPI 时失败。TSYS-04 已覆盖四边穿透展开/收回；TSYS-05 已覆盖双屏拖动与重启恢复；TSYS-09 已覆盖原生字体、主题、材质截图；TSYS-13 已有短时探针数据。这些部分覆盖不代表对应整项验收完成。

## 用例

| ID | 需求 | 层 | 前置 | 步骤 | 期望 | 证据 | 状态 |
|---|---|---|---|---|---|---|---|
| TSYS-00 | SYS-F07/08/09 | L0 | 未来打包产物 | 校验清单、Qt依赖、双语、权限、无用户数据 | 可安装，权限与实际用途一致，无未声明依赖 | package-audit | 清单结构/权限与 Qt manifest 条款检查通过；完整包审计未执行 |
| TSYS-01 | SYS-F01 | L3 | 真实宿主启用 | 输入sys，查看摘要并复制指标 | 无查询副作用，复制正确且无多余字段 | host-query/copy | 安装版 WinShun 隔离 profile 中输入 `sys ` 已展开结果且无崩溃；独立协议返回全部 P0 字段和 4 个 DXGI 适配器，QML bridge errors 清零。复制/详情与设置视觉未测 |
| TSYS-02 | SYS-F02/12 | L2 | fake正常/不支持/失败源 | 各组插入多适配器、缺失电池、失败GPU | 状态区分，其他指标保留，不把缺失当0 | fake-matrix | 核心差值状态覆盖部分完成；各数据源 fake matrix 未执行 |
| TSYS-03 | SYS-F03 | L2 | fake时钟与采样源 | 同时打开三个消费者，计数采样次数 | 同快照一致，无重复采样器 | sample-count | 代码使用单个 Sampler/Snapshot；多消费者计数测试未执行 |
| TSYS-04 | SYS-F04/09 | L4 | 解锁桌面 | 开启/锁定/穿透；用sys编辑；切前台应用 | 默认关闭，能恢复编辑，无焦点抢夺 | overlay-video/screens | 四边穿透展开/收回通过；完整焦点矩阵待测 |
| TSYS-05 | SYS-F05/09 | L4 | 混合DPI双显示器 | 拖动保存、重启、拔出显示器、改变缩放 | 位置恢复或安全回工作区，偏好不丢失 | monitor-matrix | 双屏拖动和重启恢复通过；原生 mixed-DPI 与热拔插待测 |
| TSYS-06 | SYS-F06/12 | L1 | fake单调时间/计数 | 首样本、回绕、时间零差、休眠大间隔 | 正确基线与陈旧状态，无负率/伪峰/除零 | delta-tests | 核心单测通过：首样本、回绕、零时间差、陈旧状态；休眠场景未执行 |
| TSYS-07 | SYS-F07/11 | L3 | 普通权限，监测网络与文件 | 打开各组；导出默认摘要后取消第二次导出 | 无自动联网/扫描/提权；导出脱敏，取消不写 | privacy-audit | 未执行 |
| TSYS-08 | SYS-F08 | L3 | 采样/overlay运行 | 停用、退出、模拟慢源后退出 | 释放线程计数器窗口，无残留进程 | process-before-after | 独立协议 shutdown 可应答退出；WinShun 停用/进程树实测未执行 |
| TSYS-09 | SYS-F09 | L4 | 明暗/高对比/中英/读屏 | 键盘遍历、缩放至200%、长字段 | 焦点可见，无裁切陷阱；动态值不连续播报 | ui-matrix | 原生主题/字体/材质截图已检查；完整无障碍待测 |
| TSYS-10 | SYS-F10 | L4 | 窗口/无边框/独占全屏应用 | 切换全屏与隐藏设置 | 在已测范围按设置隐藏/恢复，不误改偏好 | fullscreen-matrix | 未执行 |
| TSYS-11 | SYS-F11 | L2 | fake敏感字段与写入失败 | 默认导出、显式包含字段、取消、磁盘拒绝 | 默认脱敏，用户选择有效，失败不丢页面 | export-tests | 未执行 |
| TSYS-12 | SYS-F06/12 | L4 | 真机双网卡/休眠 | 传测试数据、断开网卡、睡眠唤醒 | 单位/身份正确，重建基线，无重复汇总 | resume-network | 未执行 |
| TSYS-13 | SYS-F02/03/06 | L5 | 可复现空载与负载场景 | 对比禁用/启用/三消费者/隐藏配置 | 记录CPU/内存/唤醒峰值，依据峰值+余量定预算 | budget-report | 已记录短时原生 CPU/GPU/帧测量；完整宿主预算待测 |

## 分层与回归门槛

L0 静态产物；L1 纯核心；L2 fake接线；L3 真实宿主；L4 真机交互；L5 预算与发布。GPU指标需与可比较的系统工具核对采样口径，不能要求瞬时数字完全相同。任何P0失败、崩溃、误导0值或穿透无法恢复均阻塞发布。既有switches与宿主搜索入口做定向回归，确认插件互不依赖。

## 待验证

跨设备 GPU 驱动覆盖、原生 mixed-DPI、外部录屏、完整无障碍和宿主资源预算待测。导出和自动全屏隐藏未实现。
