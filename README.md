# ra9fael.sysinfo

[English](README.en.md)

Windows 系统信息查询与原生液态玻璃桌面监视窗。当前版本 `1.4.0-dev`，开发中；已在 WinShun 安装并回读为启用、运行中。

## 桌面监视窗

显示 CPU、内存、GPU、磁盘忙时、网络上传和下载。普通与紧凑模式都支持自定义网格、居中文字和波形。

- 格子顺序：`cpu,memory,gpu;disk,netUp,netDown` 表示两行三列；分号或换行分行，逗号分列，`cpu,,memory` 保留空格。
- 波形：`off` 关闭；`inline` 折线、`dotted` 采样点和 `bars` 细柱位于文字下方，与文字分离；`wash` 将面积波形铺在文字背后，文字叠加其上。
- 波形颜色：一个 `#RRGGBB` 统一颜色，或六个逗号分隔的颜色，依次对应 CPU、内存、GPU、磁盘、上传、下载，不随格子顺序改变。
- 字体：可选 family 与 70–180% 字号；过长内容按实际测量缩小适配。
- `tiny`：单行文字，28px 行高、4px 外留白，底部波形高 6px；`wash` 不占额外高度。保留列宽、空格子及标签/数值/单位开关；大字号缩小适配固定行高。圆角最大 10px，尺寸均随整体缩放变化。
- 配色：`system` 跟随系统，`light` / `dark` 固定玻璃色调和文字颜色。
- 玻璃：`liquid` 液态、`frosted` 磨砂、`square` 方格压花。方格尺寸 8–80 像素，折射设置控制强度。
- 交互：拖动定位、锁定、点击穿透、四边半圆隐藏与 hover 展开；右键不弹菜单。点击穿透时仍可 hover 唤醒。位置与显示器名称持久化。
- 动效：指针附近局部折射、按压缓动和拖动流动；动画开关控制动态响应。

采样间隔 `intervalMs` 为 500–5000ms，默认 1000ms，控制系统指标与波形历史更新。历史保留 60 次采样，因此时间跨度随间隔变化。玻璃背景采集和动画独立调度；16ms 定时器不代表实际 60fps。

`captureVisible` 控制窗口是否允许被捕获。开启时，背景采集会临时排除自身并同步 DWM，再恢复捕获可见性。

## 架构与构建

插件使用 WinShun SDK、C++23 和 Qt 6.12 Core/Gui 管理协议、事件循环、设置与采样。桌面监视窗使用 Win32 HWND、D3D11/DXGI、DirectComposition、HLSL 和 DirectWrite/Direct2D。

两个依赖均为固定提交的 Git 子模块：

- [liquid-glass-cpp](https://github.com/ra9fael/liquid-glass-cpp)：独立渲染库，负责窗口、背景采集、材质和贴边动画；不依赖 Qt 或 WinShun。来源声明见其 `THIRD-PARTY-NOTICES.md`。
- [hwinfo](https://github.com/lfreist/hwinfo)：2.0.0，提交 `acdc2eb4a9109b4ea6bbe207e03d9da33bc16fab`，MIT；许可证位于 `third_party/hwinfo/LICENSE`。

在 Visual Studio 开发者终端中运行，`CMAKE_PREFIX_PATH` 指向本机 Qt 6.12 MSVC 安装目录：

```powershell
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<Qt安装目录>"
cmake --build build
ctest --test-dir build --output-on-failure
```

运行测试时 Qt 的 `bin` 目录须在 PATH 中。渲染测试需要交互式 Windows 桌面和 D3D11 设备；构建输出 `sysinfo.exe` 位于源码目录。

## 验证与权限

权限为 `clipboard:write` 和 `ui`。采集不联网、不启动外部进程、不扫描用户文件、不提权、不修改系统设置。hwinfo、Win32 IP Helper 与 PDH 提供硬件信息、网络速率及 GPU/磁盘利用率；字段可用性取决于系统和驱动。

- [功能与设计](plugin-design.md)
- [UI 规格](UI-SPEC.md) · [测试计划](TEST-PLAN.md)
- [原生桌面集成结果](docs/native-integration-results.md)
- [材质、采样与性能验证](docs/native-material-validation.md)

采样间隔、四方向穿透 hover、双屏拖动与位置恢复已验证。短时探针中，捕获可见模式绘制调用中位约 10ms，实际绘制间隔约 33ms；GPU 绘制中位约 0.06–0.08ms。测量条件与边界见验证记录。

## 已知限制

当前两屏原生 DPI 均为 96，未验证原生 mixed-DPI。外部录屏、独立动态背景负载、物理拖动性能轨迹及完整无障碍验收尚未覆盖。独立库已公开并验证克隆；完整插件跨机器构建尚未验证。导出与全屏自动隐藏未实现，硬件传感器字段不保证每台设备可用。
