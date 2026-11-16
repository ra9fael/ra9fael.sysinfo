# Liquid Glass 原生渲染技术分析与实现对比

分析日期：2026-11-16。对象为三个指定仓库的源码快照，以及 `ra9fael.sysinfo/native_overlay.cpp` 和 `sysinfo_ui.cpp`。结论基于源码与坐标公式分析；本次未编译、运行三个参考项目，也未测量帧率、显存或 GPU 时间。

## 1. 结论

**Windows 桌面采集与 D3D11 材质实现以 liquidDX11 为主参考；控件材质组织、覆盖率与缩放参考 imgui-liquid-glass；连续形变、运动厚度和高光参考 Glasscope。** 三者处于不同的平台和渲染层级，不能将其背景获取方式视为可互换模块。

当前 Overlay 的玻璃背景是一次覆盖整个窗口的绘制，并非按 CPU、内存等指标分别生成玻璃纹理。但它存在会造成分区折射、倒置和重影的明确数学错误：

1. 全屏顶点 shader 的纹理 Y 坐标与 D3D11 屏幕坐标相反，且该 shader 被所有模糊 pass 和最终玻璃 pass 共用。
2. 边缘权重写反：内部为 1，轮廓边缘趋近 0。大面积内部受到本应局限于边缘的法线折射，距离场法线在矩形内部切换方向，能够形成分区观感。
3. 原始背景与经过奇数次垂直翻转的模糊背景不在同一坐标方向，混合时不能对齐。
4. 色散阶段覆盖了已经模糊混合过的 R/B 通道，使只有 G 通道保留混合结果。
5. 缩放只影响窗口尺寸、边距和圆角，未贯穿文字、内部留白、行距、模糊核和折射宽度。

**双纹理、多个 pass、按控件绘制，本身都不是错误。** 两个 UI 参考项目明确采用双背景纹理；Glasscope 甚至把多个形状平滑合成为连续透镜。判断标准是采样坐标、背景来源、形状边界和合成约定是否一致。

四角黑色的具体根因尚未由运行证据确定。当前代码存在 HWND 透明合成、resize 返回值和 D3D/D2D 状态管理方面的核查点，但不能仅凭 `discard` 就认定它必然产生黑角。

## 2. 源码版本与平台边界

| 项目 | 本次源码 commit | 所在平台与作用 |
|---|---|---|
| [imgui-liquid-glass](https://github.com/SoyBeanMilkx/imgui-liquid-glass/tree/7dfa072d2b69ad57c031f32feaf111148e65116d) | `7dfa072d2b69ad57c031f32feaf111148e65116d` | Android ImGui 控件与 Overlay；OpenGL ES / Vulkan |
| [liquidDX11](https://github.com/poncippg-spec/liquidDX11/tree/00c2b2b19eb5523abde473b2c08220326d4146d4) | `00c2b2b19eb5523abde473b2c08220326d4146d4` | Windows Win32 / DXGI Desktop Duplication / D3D11 / HLSL |
| [Glasscope](https://github.com/Horizon0427/Glasscope/tree/29dd79133d08538d2b09f2d2e7b2608888b866f0) | `29dd79133d08538d2b09f2d2e7b2608888b866f0` | Hyprland 合成器插件，GLES 液态放大镜与取色器 |

本地快照位于 `build/glass-reference-analysis/{imgui-liquid-glass,liquidDX11,Glasscope}`。

| 维度 | imgui-liquid-glass | liquidDX11 | Glasscope | 当前 sysinfo |
|---|---|---|---|---|
| 背景来源 | 宿主当前 framebuffer / Vulkan 图像 | DXGI 捕获桌面输出 | 合成器当前 framebuffer 的局部副本 | DXGI 输出的窗口周边 crop |
| 避免自身反馈 | 默认在自身 UI 绘制前保存背景；可选 PreviousContent | 默认排除自身 HWND；F11 可解除排除 | 在透镜绘制前复制场景 | 抓帧时临时切换 affinity |
| 背景更新 | 随宿主渲染帧 | 按采集节流获取新帧 | 随合成器 render pass / damage | dirty 或拖动时采集 |
| 模糊 | 半分辨率、可分离高斯 | 六级下采样及上采样的 Dual Kawase | 核心透镜路径直接采样，无独立模糊链 | 直接缩至 1/4，再两次同尺寸 Kawase |
| 背景纹理 | 原始 + 模糊 | Soft + Heavy | 单场景纹理，线性或最近邻 | 原始 + 模糊 |
| 玻璃单元 | GlassRequest，圆角矩形/胶囊/圆形 | Primitive 队列，可合并 blob | 连续透镜，可形变 | 整个 HWND 一块 |
| 透明约定 | 所查 GL 路径为 straight alpha 混合 | premultiplied alpha | 插件路径 premultiplied alpha | DComp premultiplied swap chain |
| 缩放依据 | density 转为物理参数 | 启动 DPI、窗口尺寸、独立 render scale | monitor scale 显式转换 | 部分属性使用 Setting.scale |

## 3. imgui-liquid-glass：共享背景与逐控件材质

### 3.1 渲染顺序

OpenGL 主路径位于 `src/ui/renderer/opengl/OpenGLRenderer.cpp:51–79`：

1. 根据当前 context 和 surface 尺寸建立资源。
2. 生成 ImGui draw data 和玻璃请求。
3. 在真正渲染 ImGui 之前调用 `prepareBackdrop()` 保存已有 framebuffer。
4. 插入玻璃 draw callbacks，再渲染 UI。

`OpenGLGlassBackend.cpp:63–89` 使用 `glBlitFramebuffer` 复制背景，生成共享模糊纹理。`prepareRequestBackdrop()` 的 `PreviousContent` 分支允许某个后绘制控件采样已经绘制的内容。这是有明确顺序的嵌套玻璃，不等同于无控制地采回上一帧自身。

因此，该项目并未提供 Windows 的“桌面透过独立 HWND”方案。它能在宿主渲染过程里拿到干净背景，平台能力与 DXGI 桌面捕获不同。

### 3.2 模糊

`GlassBlurPolicy.cpp:23–56` 根据物理模糊半径、质量档位生成高斯权重及成对偏移。sigma 在半分辨率工作纹理中定义，权重归一化。

`OpenGLGlassBlurPass.cpp:40–49` 执行三个 pass：

```text
原始背景 → 半分辨率降采样 → 水平高斯 → 垂直高斯
```

每个 pass 使用其实际输入纹理的宽高计算 texel；关闭混合、深度、裁剪等可能影响全屏绘制的状态。

### 3.3 折射、形变与色散

`shaders/opengl/liquid_glass.frag:43–138`：

- 在控件局部坐标中计算圆角矩形/圆形 SDF。
- `jellyHalfSize()` 与 `jellyCoordinate()` 同时改变轮廓尺寸及坐标，实现挤压形变。
- 深入内部超过 `refractionHeight` 时直接采背景；只在边缘带内施加折射。
- 边缘位移采用 `circleMap(depth) = 1 - sqrt(1-depth²)`，强度与方向由参数和形状梯度决定。
- 使用七个光谱位置组合 RGB，而不是只偏移两个颜色通道。
- `sampleContent()` 明确允许原始/模糊背景混合。

OpenGL 版本在采样时执行 `uv.y = 1 - uv.y`；Vulkan 版本的对应函数没有这一步。它说明**纹理翻转取决于具体 API 与坐标路径，不能跨后端照搬符号**。

### 3.4 边缘覆盖与合成

`glass_math.glslinc:20–43` 同时提供基于 `fwidth` 的覆盖率，以及局部直线边界的像素面积覆盖率。小尺寸控件使用更细的覆盖计算，避免细边闪烁。

导数在任何 early return 之前计算；颜色、轮廓覆盖率与外阴影合并后再输出。所查 GL 后端使用 `SRC_ALPHA / ONE_MINUS_SRC_ALPHA`，与其 straight-alpha 输出配套，不能直接搬到当前 DComp 的 premultiplied 输出路径。

### 3.5 可借鉴内容

适合移植的是边缘函数、圆角覆盖率、控件请求的数据组织、density 的统一使用，以及“背景先准备、前景后绘制”的约定。Android Hook、触摸及 IME 桥接与本插件无关。

## 4. liquidDX11：最接近本项目的平台实现

### 4.1 桌面捕获

`src/glass/backdrop.cpp:18–50,176–218,266–291`：

- 初始化即对 HWND 设置 `WDA_EXCLUDEFROMCAPTURE`。
- 枚举当前 D3D 设备 adapter 上的输出，按窗口中心选择输出。
- 首帧捕获可等待 250ms，已有首帧后使用非阻塞采集。
- timeout 保留已有纹理；`DXGI_ERROR_ACCESS_LOST` 释放 duplication，下次调用重建。
- `LastPresentTime` 用于区分实际桌面图像更新；仅在新图像到达时重新复制和模糊。
- 背景纹理按输出尺寸建立，而非对每个控件分别抓屏。

它保存输出原点，最终使用 `window_origin + client_position` 定位桌面纹理。

**捕获可见性存在需要独立处理的边界。** `main.cpp:296–311` 在持续采集的同时，F11 将 affinity 从 `0x11` 切为 `0`。所查路径没有“剔除自身像素”的独立来源，也没有双帧消费协议。不能据此宣称它在截图可见模式下天然免疫反馈，更不能把两次 AcquireNextFrame 当作来自该参考的确定解法。

### 4.2 Dual Kawase

`backdrop.cpp:103–119,220–264`：

- 六层 `down`：1/2、1/4、1/8、1/16、1/32、1/64。
- 逐层上采样恢复到全尺寸 Heavy。
- Soft 由原始桌面通过轻量采样生成。
- blur pass 显式接收 `src_w/src_h`，偏移以当前输入尺寸为单位。
- 同一份 Soft/Heavy 提供给多个材质 Primitive。

这与当前“降到 1/4 后来回两次”的实现不同。后者不能称为同等的 Dual Kawase 金字塔。

### 4.3 材质模型

`src/glass/shaders.h:167–179,216–359`：

1. 使用圆角矩形 SDF，可调角部指数；可通过 smooth union 合并两个 blob。
2. 根据 SDF 数值差分计算二维法线，再由 bevel 斜率构造三维法线。
3. 用三维法线计算 Fresnel、双方向光和镜面高光。
4. 折射边缘权重为 `1 - smoothstep(0, refr_band, -d)`：轮廓处为 1，内部衰减为 0。
5. 折射由 `+nrm * edge² * strength` 与独立中心 lens 项共同决定。
6. RGB 分别采样，但每个通道均通过一致的 Soft/Heavy 混合函数。
7. 额外 frost 邻域采样柔化边缘；随后处理色调、饱和度、亮度、对比度、grain、高光和内阴影。
8. 最终 alpha 由 `shape * fade` 与阴影覆盖率决定，返回 premultiplied RGB。

它既使用双纹理，也使用沿正法线的折射。不存在“必须单纹理、必须负法线才正确”的通用规则。

### 4.4 窗口、绘制状态与缩放

`main.cpp:167–184` 使用 DPI awareness、启动显示器 DPI、`WS_EX_LAYERED`、`SetLayeredWindowAttributes` 和 `DwmExtendFrameIntoClientArea`。这是其 HWND 呈现路线；当前 sysinfo 用 DirectComposition，不能把两套窗口参数零散混搭。

`glass.cpp:366–378,452–479` 显式建立并绑定 premultiplied blend、无深度、无背面剔除和 scissor 状态。每个 Primitive 有局部几何、窗口坐标和裁剪矩形；`render_scale_` 用于渲染分辨率缩放。

参考项目展示了启动 DPI 和 render scale 的分离，但本次所查捕获实现仅在 duplication 建立时选择输出，不能把它当作完整的跨屏热切换实现直接替换本项目现有逻辑。

### 4.5 适合本项目的使用方式

将其捕获状态机、正确的全屏 UV、Dual Kawase、材质坐标、SDF/覆盖率以及 pass 状态设置作为主要移植依据。WinShun 协议、采样与设置仍由现有 C++ 管理，前景文字继续使用 DirectWrite。无需为了采用 HLSL 引入整套 ImGui demo。

## 5. Glasscope：合成器内的连续液态透镜

### 5.1 背景来源与局部复制

`src/render/lens_renderer.cpp:15–97` 依据透镜及拖尾范围计算 copy 区域，将当前 framebuffer 的局部内容复制到纹理，然后在该区域绘制透镜。

`scene_copy.cpp` 以 128 像素为容量档位增长纹理，避免每个细小尺寸变化都重新分配。采样同时传递实际有效区域 `textureMax`，不会将额外容量误当成有效桌面。

这里的局部 copy 和容量分档是资源管理，**不是把玻璃切成 128 像素方块**。

Hyprland 提供场景绘制阶段的访问权，因此透镜可以在绘制自身之前得到源像素。Windows 普通 HWND 无法直接获得同等的合成器内部接口。

### 5.2 真正的轮廓形变

`shaders/shape_follow.glsl:25–67` 沿三次 Bézier 拖尾路径取 10/12 个样本，以局部圆距离和 smooth minimum 构成连续形状。固定模式另有 `shape_pinned.glsl`，运动更新有阻尼弹簧实现。

这与“轮廓保持矩形，只让采样坐标晃动”不同。轮廓、法线、厚度、高光会一起响应运动，才能获得连贯的液体感。

### 5.3 光学

`shaders/optics.glsl`：

- 中心为 `zoom + domeScale` 放大。
- 外缘、窄 rim 和 inner shell 分别构造强度。
- 用运动方向、拖尾拉伸与压缩量调整光学厚度。
- 边缘法线折射、切线 wobble、色散、细丝高光及聚光带共同构成边缘视觉。
- tint 后通过 `preserveLuminance()` 保持来源亮度，减少玻璃变成脏色板的倾向。
- 透镜主路径没有 Gaussian/Kawase 模糊。`lens_pass_element.cpp:57–62` 也不请求合成器 live/precompute blur。

不能把 Glasscope 归为“统一磨砂背景”的实现；它的目标是保留内容可辨识度的放大镜。

### 5.4 缩放与透明

monitor scale 明确用于中心、半径、边宽、拖尾和颜色带宽。插件 shader 输出 `refracted * alpha, alpha`，渲染器使用 `ONE / ONE_MINUS_SRC_ALPHA`。

适合借鉴的是局部有效采样区域、运动几何与材质一致性、保亮度染色及显式缩放；监视器面板不需要直接复制放大倍率和长拖尾。

## 6. 当前 Overlay 的源码问题

### 6.1 确定：D3D11 的 Y/UV 方向相反

当前 `native_overlay.cpp:258`：

```hlsl
o.position = float4(p[id], 0, 1);
o.uv = (p[id] + 1) * .5;
```

D3D11 viewport 顶部对应 NDC `y=+1`，而桌面纹理顶部对应 `v=0`。当前在顶部却得到 `v=1`。

liquidDX11 的 `shaders.h:18–24` 明确写为：

```hlsl
o.uv = uv;
o.pos = float4(uv.x * 2 - 1, 1 - uv.y * 2, 0, 1);
```

当前三个 blur pass 都复用错误 VS，奇数次翻转后的 blurPing 与原始 backdrop 方向不同。最终 pass 再将窗口局部 UV 映射到桌面 crop；crop 尺寸与窗口尺寸不同，不能简单依靠两次翻转相消恢复正确位置。

**这能确定背景存在垂直倒置及两条采样路径不对齐。** 若用户所说的“反向”还包括水平镜像，需要实际画面对照，源码中的这一错误本身证明的是上下翻转。

### 6.2 确定：边缘权重反了，内部出现方向分区

当前 `native_overlay.cpp:87–98`：

```hlsl
float d = sdSquircle(...); // 内部 d < 0
float edge = 1 - smoothstep(-width, 0, d);
```

代入两个位置：

| 位置 | 当前 edge | 期望的边缘带 |
|---|---:|---:|
| 深入内部，d ≤ -width | 1 | 0 |
| 轮廓边界，d = 0 | 0 | 1 |

随后 `pow(edge,1.55)` 驱动折射，导致内部持续受到偏移。矩形距离场内部的最近边会发生切换，其梯度形成朝向不同的区域；大面积内部折射会让背景看起来被分成几片。

因此代码没有按指标切玻璃，但数学上的分区偏移确实存在。按文字绘制的 veil 和按格绘制的 wash 还可能增强局部块状感，它们不是独立玻璃材质。

### 6.3 确定：R/B 绕过模糊结果

`native_overlay.cpp:103–109` 先计算 `lerp(raw, softened, blurMix)`，随后用原始 backdrop 的 R/B 覆盖结果。即使 dispersion 为 0，R/B 也来自原始图，只有 G 保留模糊混合。

这会造成通道间清晰度不一致。liquidDX11 对三个通道都使用同一个 `sampleBackdropFrost()`，避免该不一致。

### 6.4 确定：模糊 texel 和设置语义不匹配

`runBlur()` 的常量始终来自原始 backdrop 的宽高。后两个 pass 输入已是 1/4 尺寸纹理，却仍使用原始尺寸的 texel，偏移单位相差约四倍（整数尺寸取整时略有偏差）。

而 blur 设置没有改变这三个 pass 的采样核或层数，主要改变最终混合量和边缘参数。因此“模糊强度”实际混合了清晰度、折射带宽等行为。

### 6.5 确定：透明度又叠加了真实桌面

最终 shader 输出 alpha 为 `lerp(.74,.28,opacity) + edge项`。其 RGB 已经包含采到的桌面，DComp 又把这幅折射桌面半透明叠到真实桌面上：

```text
最终显示 ≈ alpha × 折射后的背景 + (1-alpha) × 未折射的真实桌面
```

两者位置或时间不一致时，会出现双轮廓。这是另一条重影来源，区别于“采回自身文字”。

liquidDX11 正常完整显示时以 shape 覆盖率和 fade 输出 alpha，材质 tint_opacity 在 RGB 内混合；Glasscope 稳定显示时也以 mask/reveal 控制覆盖。移植时应分开“材质染色强度”和“整个窗口淡出透明度”。

### 6.6 确定：缩放不完整

`sysinfo_ui.cpp:95–113,218–221` 缩放窗口/面板尺寸、边距和 radius，却只将独立的 fontSize 传入字体百分比。

`native_overlay.cpp:306–324,634–678` 中字体基础大小、pad、文字 y 偏移、veil 留白等仍为固定值。改变整体 scale 后，外形与内容不同比例变化。

另一个独立问题是 `availableArea()` 使用 QScreen 逻辑工作区，而自由拖动使用 Win32 物理工作区。多屏 DPI 不同时，预设定位与自由拖动的坐标来源不一致。整体 scale、系统 DPI 与渲染分辨率应分别定义，不应互相替代。

### 6.7 确定：捕获更新与错误恢复缺项

当前 `captureBackdrop()` 在静止且非 dirty 时不抓背景；但 `draw()` 仍每次调用 `runBlur()`。因此桌面静止透射会过期，重复模糊却仍在发生。

AcquireNextFrame 失败分支未处理 ACCESS_LOST，`ensureDuplication()` 在 monitor 不变且 duplication 非空时直接复用。句柄失效后不能通过同显示器下一帧自然重建。liquidDX11 有明确的释放/重建分支。

`captureVisible=true` 的临时排除是否拿到已剔除自身的帧，尚需采集证据；现有源码没有验证帧属于哪次排除状态。**双帧消费不构成已证明的解决方案。**

### 6.8 四角黑色：确定缺项与待验证原因

已有正确设置：swap chain 是 `DXGI_ALPHA_MODE_PREMULTIPLIED`，玻璃 pass 清为 `(0,0,0,0)`。在正确透明合成下，轮廓外 `discard` 应保留透明，不会天然变黑。

需要针对黑角核查：

- HWND 使用普通 WS_POPUP 扩展样式，DComp 透明区域下方的 HWND redirection 表面如何处理；当前既没有 WS_EX_NOREDIRECTIONBITMAP，也没有 liquidDX11 的 layered + DWM 扩展路径。
- `ResizeBuffers()`、RTV/D2D target 创建、`EndDraw()` 和 `Present()` 的部分结果未检查；尺寸与实际 backbuffer 是否同步尚无证据。
- 当前 D3D pass 没有显式设置 blend、rasterizer、depth-stencil/input layout 等完整状态；与 D2D 共用 device 后，需要确认状态是否影响后续绘制。
- 边缘导数在 `discard` 后计算，而且轮廓没有平滑 coverage；这影响边缘稳定性，但不能单独解释四角全部黑色。

验证应同时读取 swap chain 圆角 RGBA 和实际桌面合成截图：若 RGBA 为零但桌面显示黑，调查 HWND/DComp 透明合成；若 alpha 非零或尺寸不符，调查渲染状态和 resize。两条结果会导向不同修法。

## 7. 建议采用的技术路线

### 7.1 保留的平台架构

```text
WinShun SDK / 设置 / 资源采样
                  ↓
Win32 HWND + 显式物理坐标
                  ↓
DXGI 背景采集 → 背景有效区域与更新状态
                  ↓
共享 Soft / Heavy 模糊背景
                  ↓
单一面板 SDF → 边缘折射/色散 → 光照/染色 → 覆盖率合成
                  ↓
DirectWrite 前景文字与指标图形
                  ↓
DirectComposition 呈现
```

指标网格仅负责内容排版。整个监视器面板保持一块连续玻璃，共享背景坐标；只有产品确实要求独立浮起的控件时，才增加独立 Primitive。

### 7.2 实施顺序与来源

1. **坐标与边缘函数。** 对齐 liquidDX11 的 D3D UV 和局部/窗口/桌面坐标关系；修正 edge 定义，同时恢复原始与模糊背景的同向采样。
2. **模糊和 RGB 采样。** 按 liquidDX11 建立 Soft/Heavy 一致采样函数及 Dual Kawase；每个 pass 使用自己的输入尺寸。局部 crop 可继续使用，但采样 padding 必须覆盖实际折射和模糊范围。
3. **轮廓与透明合成。** 使用 coverage，不再把 glass 材质强度直接等同于 HWND 透明度；按实测选择与 DComp 一致的 HWND 透明路径，统一 resize 生命周期及错误处理。
4. **完整缩放。** 给布局、字体、圆角、折射带宽、模糊半径和 hit-test 建立共同的逻辑到物理转换；把用户 scale、显示器 DPI 和 render scale 区分开。
5. **背景更新。** 参考 liquidDX11 的 timeout/ACCESS_LOST 状态机，在新背景或模糊参数变化时运行模糊。captureVisible 单独验证，保留其既有语义。
6. **液态运动。** 基础静态画面正确后，再按 Glasscope 的形状—法线—厚度—高光联动引入克制形变；不是只往 UV 加正弦波。

### 7.3 有针对性的验证

| 检查 | 能发现的具体失败 | 失败后处理 |
|---|---|---|
| 带上下标识与单行文字的背景，分别查看原图/Soft/Heavy/最终图 | Y 翻转、crop 偏移、双背景不对齐 | 修正对应 pass 的坐标转换 |
| 关闭色散并改变 blur | 通道清晰度不一致、模糊设置不生效 | 统一 RGB 采样与模糊参数 |
| 查看中心与四边折射 | 内部发生分区、中心文字扭曲 | 修正 edge/normal 使用范围 |
| 比较圆角 RGBA 与桌面截图 | 黑角来自内容或 HWND 合成 | 分别处理渲染/窗口透明路径 |
| 插件 scale 连续变化后恢复 | 内外尺寸不同步、字体/边距不同比例 | 修正模型与资源更新顺序 |
| 两屏不同 DPI 下定位、拖动、贴边 | 逻辑/物理坐标混用 | 修正边界转换 |
| captureVisible 开/关下分别检查 backdrop | 自身内容进入采样源 | 调整捕获策略并验证帧时序 |
| 静止窗口后播放动态背景 | 背景不更新或自身反馈积累 | 修正采集更新条件 |

## 8. 使用边界与许可

- 本报告的确定问题来自代码及公式，未将参考项目的演示视频当作当前机器上的实测结果。
- imgui-liquid-glass 的默认示例依赖仓库未附带的 Android Hook/触摸二进制，不能直接在 Windows 编译运行整个 demo。部分 shader 明确标注来自 AndroidLiquidGlassView 的 MIT 代码；本次快照未发现仓库级 LICENSE 文件，整套控件代码的复制许可需要另外确认。
- liquidDX11 的 README 声明原始应用代码 MIT，第三方 ImGui、FreeType 与字体按各自许可处理。移植时应保留相应声明。
- Glasscope 为 BSD-3-Clause；直接移植算法代码须保留版权、条件及免责声明。其依赖 Hyprland 0.57.1，且明确只支持未旋转输出。
- 三者均没有给当前 sysinfo 的 captureVisible、混合 DPI、多显示器边缘隐藏提供现成的完整组合实现；这些行为必须结合本项目接口验证。

## 源码索引

- [imgui-liquid-glass：OpenGL 背景与逐控件合成](https://github.com/SoyBeanMilkx/imgui-liquid-glass/blob/7dfa072d2b69ad57c031f32feaf111148e65116d/src/ui/widget/effects/glass/backend/opengl/OpenGLGlassBackend.cpp)
- [imgui-liquid-glass：材质 shader](https://github.com/SoyBeanMilkx/imgui-liquid-glass/blob/7dfa072d2b69ad57c031f32feaf111148e65116d/src/ui/widget/effects/glass/shaders/opengl/liquid_glass.frag)
- [imgui-liquid-glass：高斯模糊策略](https://github.com/SoyBeanMilkx/imgui-liquid-glass/blob/7dfa072d2b69ad57c031f32feaf111148e65116d/src/ui/widget/effects/glass/GlassBlurPolicy.cpp)
- [liquidDX11：背景采集与 Dual Kawase](https://github.com/poncippg-spec/liquidDX11/blob/00c2b2b19eb5523abde473b2c08220326d4146d4/examples/example_win32_directx11/src/glass/backdrop.cpp)
- [liquidDX11：HLSL](https://github.com/poncippg-spec/liquidDX11/blob/00c2b2b19eb5523abde473b2c08220326d4146d4/examples/example_win32_directx11/src/glass/shaders.h)
- [liquidDX11：材质队列与管线状态](https://github.com/poncippg-spec/liquidDX11/blob/00c2b2b19eb5523abde473b2c08220326d4146d4/examples/example_win32_directx11/src/glass/glass.cpp)
- [Glasscope：局部场景复制与合成](https://github.com/Horizon0427/Glasscope/blob/29dd79133d08538d2b09f2d2e7b2608888b866f0/src/render/lens_renderer.cpp)
- [Glasscope：光学](https://github.com/Horizon0427/Glasscope/blob/29dd79133d08538d2b09f2d2e7b2608888b866f0/shaders/optics.glsl)
- [Glasscope：连续拖尾形状](https://github.com/Horizon0427/Glasscope/blob/29dd79133d08538d2b09f2d2e7b2608888b866f0/shaders/shape_follow.glsl)
