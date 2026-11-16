// 采样：CPU、内存、GPU、磁盘忙时、网络收发，外加 60 点的历史给波形用。
//
// 数据源：hwinfo（GetSystemTimes / GlobalMemoryStatusEx / DXGI 枚举 / 电池）、
// GetIfEntry2（字节计数差值）、PDH（GPU Engine 利用率、PhysicalDisk 空闲时间）。
// 全部普通权限，不联网，不读用户文件。
#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "sysinfo_core.h"

namespace sysinfo {

enum class Metric { Cpu, Memory, Gpu, Disk, NetUp, NetDown };
constexpr int kMetricCount = 6;

struct Reading {
    Status status = Status::Sampling;
    double value = 0; // 百分比，或网络字节/秒
};

// 一次采样的完整快照：监视窗、搜索行和测试都只看它。
struct Sample {
    std::uint64_t at = 0; // 毫秒，从采样器启动算起
    std::array<Reading, kMetricCount> readings{};
    std::array<std::vector<double>, kMetricCount> history{};

    bool netOk = false;
    bool gpuEnumOk = false;
    bool gpuCounterOk = false;
    bool diskCounterOk = false;
    std::string os;
    std::string processor;
    std::string gpuName;
    std::string gpuError;
    std::string netName;
    std::string systemVolume;
    int adapters = 0;
    int gpus = 0;
    double ramUsedGiB = 0, ramTotalGiB = 0;
    double diskUsedGiB = 0, diskTotalGiB = 0;
    bool batteryAvailable = false;
    bool batteryCharging = false;
    int batteryPercent = -1;
};


class Sampler {
public:
    Sampler();
    ~Sampler();
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    void start();
    void stop();
    // 暂停后只更新时间戳，不碰任何计数器（监视窗关掉时用）。
    void setPaused(bool paused) { m_paused = paused; }
    bool paused() const { return m_paused; }

    virtual Sample get() const;
    std::uint64_t nowMs() const;

private:
    void run();
    void sample(Sample& s);
    void pushHistory(Sample& s);

    std::thread m_worker;
    std::atomic_bool m_running{false};
    std::atomic_bool m_paused{false};
    mutable std::mutex m_mutex;
    Sample m_value;
    std::chrono::steady_clock::time_point m_start;
};

} // namespace sysinfo
