# Native material validation

## Sampling

`sysinfo.exe --verify-sampling` exercises the real sampler, including live
interval changes. Observed intervals in milliseconds:

| Setting | Observed |
| --- | --- |
| 500 | 527, 531 |
| 1000 | 1001, 1026 |
| 5000 | 5024, 5004 |
| 500 after 5000 | 530, 527 |

## Rendering and input

Four-direction click-through hover tests exercise expansion and delayed collapse
sequentially in the same window. Desktop capture and resize assertions cover
both capture visibility modes. Visual inspection covers inline, wash, dotted,
bars, liquid, frosted and square glass; light and dark themes; Segoe UI and
Consolas at 180% in narrow, compact cells. Long strings fit their measured boxes.

Wave colors accept one shared color or six colors in CPU, memory, GPU, disk,
upload, download order. Inline uses an open line, wash uses a translucent area,
dotted uses sample points, and bars uses individual columns. History contains
60 samples, so its time span depends on the sampling interval.

References: [Rainmeter Line](https://docs.rainmeter.net/manual/meters/line/) and
[Histogram](https://docs.rainmeter.net/manual/meters/histogram/) document distinct
history representations, per-series colors and automatic range scaling.

## Performance

Eight-second observations after two seconds of warm-up, on the interactive
desktop with the probe content. CPU is a percentage of one logical CPU.
Frame duration includes Present; interval measures successive draw completions.
GPU timestamp spans blur, glass and content submission, excluding CPU capture.

| Capture visible | Scenario | CPU % | Frame median ms | Capture median ms | GPU median ms | Interval median ms |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| false | stationary | 0.00 | 0.314 | 0.137 | 0.051 | 30.573 |
| false | hover | 1.56 | 0.335 | 0.144 | 0.051 | 30.546 |
| false | moving | 0.39 | 0.325 | 0.139 | 0.145 | 30.548 |
| true | stationary | 8.57 | 10.263 | 10.108 | 0.077 | 32.814 |
| true | hover | 6.81 | 9.841 | 9.627 | 0.062 | 32.933 |
| true | moving | 4.99 | 10.371 | 10.197 | 0.063 | 32.587 |

Capture-visible frame p95 was 17.214–18.029 ms. Its synchronization dominates
frame duration. The capture strategy remains unchanged; shader execution is
small at the median. The measured cadence is approximately 30–33 ms, not 60 fps.

## Limitations

These short observations are not a hardware-independent benchmark. Windows
process CPU accounting can round very small consumption to zero. GPU timestamp
p95 ranged up to 11.570 ms and includes scheduling gaps, not just active shader
execution. Moving uses window repositioning, not a physical drag trace. Native
mixed-DPI and external recorder behavior require separate environments/tests.
