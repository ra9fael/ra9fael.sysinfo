#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sysinfo {

enum class Status { Fresh, Sampling, Stale, Unsupported, Error };
enum class RefreshClass { Static, Dynamic };

struct MetricDefinition {
    std::string id;
    std::string unit;
    std::string source;
    RefreshClass refreshClass = RefreshClass::Dynamic;
};

struct MetricCapability {
    bool available = false;
    std::string reason;
};

struct SamplePoint {
    double value = 0;
    std::uint64_t monotonicMs = 0;
    Status status = Status::Sampling;
};

struct Snapshot {
    std::uint64_t sampledAtMs = 0;
    std::map<std::string, SamplePoint> samples;
    std::map<std::string, MetricCapability> capabilities;
};

struct CounterDelta {
    Status status = Status::Sampling;
    double perSecond = 0;
};

struct Counter {
    std::uint64_t value = 0;
    std::uint64_t timeMs = 0;
};

inline CounterDelta rate(std::optional<Counter> previous, std::uint64_t value, std::uint64_t timeMs)
{
    if (!previous || timeMs <= previous->timeMs || value < previous->value)
        return {Status::Sampling, 0};
    const auto elapsedMs = timeMs - previous->timeMs;
    const auto delta = value - previous->value;
    const double perSecond = static_cast<double>(delta) * 1000.0 / static_cast<double>(elapsedMs);
    if (!std::isfinite(perSecond) || perSecond < 0)
        return {Status::Error, 0};
    return {Status::Fresh, perSecond};
}

inline Status statusAt(const SamplePoint& sample, std::uint64_t nowMs, std::uint64_t maxAgeMs)
{
    if (sample.status != Status::Fresh)
        return sample.status;
    if (nowMs < sample.monotonicMs || nowMs - sample.monotonicMs > maxAgeMs)
        return Status::Stale;
    return Status::Fresh;
}

inline std::string statusName(Status status)
{
    switch (status) {
    case Status::Fresh: return "fresh";
    case Status::Sampling: return "sampling";
    case Status::Stale: return "stale";
    case Status::Unsupported: return "unsupported";
    case Status::Error: return "error";
    }
    return "error";
}

inline std::string formatValue(double value, const std::string& unit)
{
    if (!std::isfinite(value))
        return "unavailable";
    char buffer[64] {};
    std::snprintf(buffer, sizeof(buffer), "%.0f", value);
    return std::string(buffer) + (unit.empty() ? "" : " " + unit);
}

// 监视窗的“数字 + 单位”分开给 QML：数字用 tabular 字号，单位小一号。
struct MetricText {
    std::string number;
    std::string unit;
};

inline MetricText formatPercent(double value)
{
    char buffer[32] {};
    if (!std::isfinite(value))
        return {"—", "%"};
    if (value < 10.0)
        std::snprintf(buffer, sizeof(buffer), "%.1f", value);
    else
        std::snprintf(buffer, sizeof(buffer), "%.0f", value);
    return {buffer, "%"};
}

// 输入是字节/秒。UP/DN 已表明方向，单位用 B / KBps / M / G，按 1024 进位；
// 小于 100 保留一位小数，之后取整，末尾的 .0 省略。
inline MetricText formatRate(double bytesPerSecond)
{
    if (!std::isfinite(bytesPerSecond) || bytesPerSecond < 0)
        return {"—", ""};
    static const char* units[] = {"B", "KBps", "M", "G", "T"};
    double value = bytesPerSecond;
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    char buffer[32] {};
    if (value < 100.0)
        std::snprintf(buffer, sizeof(buffer), "%.1f", value);
    else
        std::snprintf(buffer, sizeof(buffer), "%.0f", value);
    std::string number(buffer);
    if (number.size() >= 2 && number.compare(number.size() - 2, 2, ".0") == 0)
        number.erase(number.size() - 2);
    return {number, units[unit]};
}

inline std::string metricText(const MetricText& text)
{
    return text.unit.empty() ? text.number : text.number + " " + text.unit;
}

inline std::string adapterMetricId(const std::string& metricId, const std::string& instanceId)
{
    return metricId + "@" + instanceId;
}

} // namespace sysinfo
