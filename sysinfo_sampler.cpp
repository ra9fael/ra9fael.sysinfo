#include "sysinfo_sampler.h"

#include <hwinfo/battery.h>
#include <hwinfo/cpu.h>
#include <hwinfo/disk.h>
#include <hwinfo/gpu.h>
#include <hwinfo/hwinfo.h>
#include <hwinfo/os.h>
#include <hwinfo/ram.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cwchar>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <pdh.h>
#include <pdhmsg.h>

namespace sysinfo {
namespace {

using Clock = std::chrono::steady_clock;

std::string utf8(const std::wstring& s)
{
    if (s.empty())
        return {};
    const int n = ::WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0)
        return {};
    std::string r(static_cast<std::size_t>(n), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), r.data(), n, nullptr, nullptr);
    return r;
}

std::wstring widen(const char* text)
{
    if (!text)
        return {};
    const int n = ::MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (n <= 1)
        return {};
    std::wstring value(static_cast<std::size_t>(n), L'\0');
    if (::MultiByteToWideChar(CP_UTF8, 0, text, -1, value.data(), n) != n)
        return {};
    value.pop_back();
    return value;
}

template <class Q>
std::uint64_t bytes(Q q)
{
    return q.value;
}

template <class Q>
double gib(Q q)
{
    return q.to(hwinfo::ByteUnit::GiB);
}

// 一条 PDH 查询：加通配计数器、每秒收一次、取出每个实例的数值。
class PdhQuery {
public:
    ~PdhQuery() { close(); }

    bool open()
    {
        if (m_query)
            return true;
        if (PdhOpenQueryW(nullptr, 0, &m_query) != ERROR_SUCCESS) {
            m_query = nullptr;
            return false;
        }
        return true;
    }

    void close()
    {
        if (m_query)
            PdhCloseQuery(m_query);
        m_query = nullptr;
        m_counter = nullptr;
        m_open = false;
    }

    bool addWildcard(const wchar_t* path)
    {
        if (!m_query && !open())
            return false;
        m_counter = nullptr;
        if (PdhAddEnglishCounterW(m_query, path, 0, &m_counter) != ERROR_SUCCESS) {
            m_counter = nullptr;
            return false;
        }
        m_open = true;
        return true;
    }

    bool collect()
    {
        if (!m_open || !m_query)
            return false;
        return PdhCollectQueryData(m_query) == ERROR_SUCCESS;
    }

    struct Item {
        std::wstring name;
        double value = 0;
    };

    bool array(std::vector<Item>& out)
    {
        out.clear();
        if (!m_open || !m_counter)
            return false;
        DWORD size = 0, count = 0;
        PDH_STATUS status = PdhGetFormattedCounterArrayW(m_counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count, nullptr);
        if (status != PDH_MORE_DATA || size == 0)
            return false;
        m_buffer.resize(size);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(m_buffer.data());
        status = PdhGetFormattedCounterArrayW(m_counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count, items);
        if (status != ERROR_SUCCESS)
            return false;
        out.reserve(count);
        for (DWORD i = 0; i < count; ++i) {
            if (items[i].FmtValue.CStatus != PDH_CSTATUS_VALID_DATA && items[i].FmtValue.CStatus != PDH_CSTATUS_NEW_DATA)
                continue;
            out.push_back({items[i].szName ? items[i].szName : L"", items[i].FmtValue.doubleValue});
        }
        return !out.empty();
    }

    bool single(double& value)
    {
        if (!m_open || !m_counter)
            return false;
        PDH_FMT_COUNTERVALUE v{};
        if (PdhGetFormattedCounterValue(m_counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, nullptr, &v) != ERROR_SUCCESS)
            return false;
        if (v.CStatus != PDH_CSTATUS_VALID_DATA && v.CStatus != PDH_CSTATUS_NEW_DATA)
            return false;
        value = v.doubleValue;
        return true;
    }

private:
    PDH_HQUERY m_query = nullptr;
    PDH_HCOUNTER m_counter = nullptr;
    bool m_open = false;
    std::vector<unsigned char> m_buffer;
};

// GPU Engine 实例名里的 luid_0x…_0x…_phys_0_eng_0_engtype_3D：按 LUID 分组求和。
// 同一块卡上的多个引擎会重复计入同一个进程的工作，所以整组相加后 clamp 到 100。
std::wstring luidKey(const std::wstring& instance)
{
    const auto at = instance.find(L"_luid_");
    if (at == std::wstring::npos)
        return L"";
    const auto end = instance.find(L"_phys", at);
    return instance.substr(at, end == std::wstring::npos ? std::wstring::npos : end - at);
}

struct Adapter {
    std::wstring id;
    std::wstring name;
    std::uint64_t rx = 0, tx = 0;
    bool up = false;
};

std::vector<Adapter> readAdapters(bool* ok)
{
    ULONG size = 16 * 1024;
    std::vector<unsigned char> buffer(size);
    auto* list = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    ULONG rc = ::GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                                      nullptr, list, &size);
    if (rc == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(size);
        list = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        rc = ::GetAdaptersAddresses(AF_UNSPEC,
                                    GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, list,
                                    &size);
    }
    if (rc != NO_ERROR) {
        *ok = false;
        return {};
    }
    *ok = true;
    std::vector<Adapter> out;
    for (auto* a = list; a; a = a->Next) {
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK || !a->AdapterName)
            continue;
        MIB_IF_ROW2 row{};
        row.InterfaceLuid = a->Luid;
        if (::GetIfEntry2(&row) != NO_ERROR)
            continue;
        out.push_back({widen(a->AdapterName), a->FriendlyName ? a->FriendlyName : a->Description, row.InOctets,
                       row.OutOctets, row.OperStatus == IfOperStatusUp});
    }
    return out;
}

struct Disk {
    std::string name;
    double total = 0, free = 0;
};

std::vector<Disk> readDisks()
{
    const auto drives = hwinfo::disks();
    if (!drives)
        return {};
    std::vector<Disk> out;
    for (const auto& d : *drives) {
        for (const auto& mount : d.mount_points) {
            const auto space = hwinfo::disk_space(mount.path);
            if (!space)
                continue;
            out.push_back({mount.path.string(), gib(space->capacity), gib(space->free)});
        }
    }
    return out;
}

} // namespace

Sampler::Sampler()
    : m_start(Clock::now())
{
}

Sampler::~Sampler()
{
    stop();
}

void Sampler::start()
{
    bool expected = false;
    if (m_running.compare_exchange_strong(expected, true))
        m_worker = std::thread([this] { run(); });
}

void Sampler::stop()
{
    m_running = false;
    if (m_worker.joinable())
        m_worker.join();
}

std::uint64_t Sampler::nowMs() const
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - m_start).count());
}

Sample Sampler::get() const
{
    const std::lock_guard lock(m_mutex);
    return m_value;
}

void Sampler::pushHistory(Sample& s)
{
    for (int i = 0; i < kMetricCount; ++i) {
        const Reading& r = s.readings[i];
        if (r.status != Status::Fresh)
            continue;
        auto& h = s.history[i];
        h.push_back(r.value);
        if (h.size() > 60)
            h.erase(h.begin(), h.begin() + static_cast<std::ptrdiff_t>(h.size() - 60));
    }
}

void Sampler::run()
{
    std::map<std::wstring, std::pair<Counter, Counter>> prevNet;
    hwinfo::CpuSampler cpu;
    PdhQuery gpuQuery, diskQuery;
    const bool gpuQueryOpen = gpuQuery.open() && gpuQuery.addWildcard(L"\\GPU Engine(*)\\Utilization Percentage");
    const bool diskQueryOpen = diskQuery.open() && diskQuery.addWildcard(L"\\PhysicalDisk(_Total)\\% Idle Time");
    int gpuWarmup = 2, diskWarmup = 2;

    std::vector<double> history[kMetricCount];
    auto staticAt = Clock::time_point{};
    std::string os, processor;
    std::vector<Disk> disks;
    std::vector<hwinfo::Gpu> gpuDevices;
    std::string gpuError;

    while (m_running) {
        const auto begin = Clock::now();
        if (m_paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        Sample s;
        s.at = nowMs();
        for (int i = 0; i < kMetricCount; ++i)
            s.history[i] = history[i];

        // CPU
        if (const auto c = cpu.sample())
            s.readings[static_cast<int>(Metric::Cpu)] = {Status::Fresh, c->total * 100.0};

        // 内存：实时路径只需要总量和可用量，直接用 Win32 API，避免每秒
        // 重新打开硬件提供者。
        MEMORYSTATUSEX memory{sizeof(memory)};
        if (GlobalMemoryStatusEx(&memory)) {
            const double total = static_cast<double>(memory.ullTotalPhys);
            const double available = static_cast<double>(memory.ullAvailPhys);
            s.ramTotalGiB = total / (1024.0 * 1024 * 1024);
            s.ramUsedGiB = (total - available) / (1024.0 * 1024 * 1024);
            if (total > 0)
                s.readings[static_cast<int>(Metric::Memory)] = {Status::Fresh, (total - available) / total * 100.0};
        }

        // GPU 利用率（PDH）
        if (gpuQueryOpen) {
            s.gpuCounterOk = true;
            if (gpuQuery.collect() && gpuWarmup <= 0) {
                std::vector<PdhQuery::Item> items;
                if (gpuQuery.array(items)) {
                    std::map<std::wstring, double> perLuid;
                    for (const auto& item : items)
                        perLuid[luidKey(item.name)] += item.value;
                    double best = 0;
                    for (const auto& [luid, sum] : perLuid)
                        best = std::max(best, sum);
                    s.readings[static_cast<int>(Metric::Gpu)] = {Status::Fresh, std::clamp(best, 0.0, 100.0)};
                }
            } else if (gpuWarmup > 0) {
                --gpuWarmup;
            }
        }

        // 磁盘忙时 = 100 - 空闲时间
        if (diskQueryOpen) {
            s.diskCounterOk = true;
            if (diskQuery.collect() && diskWarmup <= 0) {
                double idle = 0;
                if (diskQuery.single(idle))
                    s.readings[static_cast<int>(Metric::Disk)] = {Status::Fresh, std::clamp(100.0 - idle, 0.0, 100.0)};
            } else if (diskWarmup > 0) {
                --diskWarmup;
            }
        }

        // 网络：所有“已连接”的适配器加起来
        std::vector<Adapter> adapters = readAdapters(&s.netOk);
        s.adapters = static_cast<int>(adapters.size());
        std::map<std::wstring, std::pair<Counter, Counter>> current;
        double rxTotal = 0, txTotal = 0;
        Status rxStatus = Status::Sampling, txStatus = Status::Sampling;
        std::wstring busiest;
        double busiestBytes = -1;
        for (const auto& a : adapters) {
            const auto found = prevNet.find(a.id);
            const std::optional<Counter> prx =
                found == prevNet.end() ? std::nullopt : std::optional<Counter>(found->second.first);
            const std::optional<Counter> ptx =
                found == prevNet.end() ? std::nullopt : std::optional<Counter>(found->second.second);
            const CounterDelta rx = rate(prx, a.rx, s.at);
            const CounterDelta tx = rate(ptx, a.tx, s.at);
            current[a.id] = {{a.rx, s.at}, {a.tx, s.at}};
            if (a.up) {
                if (rx.status == Status::Fresh) {
                    rxTotal += rx.perSecond;
                    rxStatus = Status::Fresh;
                }
                if (tx.status == Status::Fresh) {
                    txTotal += tx.perSecond;
                    txStatus = Status::Fresh;
                }
                if (const double moved = rx.perSecond + tx.perSecond; moved > busiestBytes) {
                    busiestBytes = moved;
                    busiest = a.name;
                }
            }
        }
        prevNet = std::move(current);
        s.readings[static_cast<int>(Metric::NetUp)] = {rxStatus, rxTotal};
        s.readings[static_cast<int>(Metric::NetDown)] = {txStatus, txTotal};
        s.netName = utf8(busiest);

        // 静态信息每 30 秒刷一次
        if (staticAt == Clock::time_point{} || begin - staticAt >= std::chrono::seconds(30)) {
            if (const auto o = hwinfo::os())
                s.os = o->name + (o->marketing_name.empty() ? "" : " (" + o->marketing_name + ")");
            if (const auto cpus = hwinfo::cpus(); cpus && !cpus->empty())
                s.processor = cpus->front().vendor + " " + cpus->front().model;
            disks = readDisks();
            gpuDevices.clear();
            gpuError.clear();
            if (const auto gpus = hwinfo::gpus()) {
                gpuDevices = *gpus;
                s.gpuEnumOk = true;
            } else {
                gpuError = gpus.error().message();
            }
            if (const auto batteries = hwinfo::batteries(); batteries && !batteries->empty()) {
                if (const auto status = hwinfo::battery_status(batteries->front().index)) {
                    s.batteryAvailable = true;
                    s.batteryCharging = status->state == hwinfo::BatteryState::charging;
                    if (status->charge)
                        s.batteryPercent = static_cast<int>(std::lround(*status->charge * 100.0));
                }
            }
            staticAt = begin;
        } else {
            s.os = os;
            s.processor = processor;
        }
        os = s.os;
        processor = s.processor;
        s.gpus = static_cast<int>(gpuDevices.size());
        s.gpuError = gpuError;
        for (const auto& g : gpuDevices) {
            if (g.type == hwinfo::GpuType::virtualized)
                continue;
            s.gpuName = g.vendor + " " + g.name;
            break;
        }
        if (s.gpuName.empty() && !gpuDevices.empty())
            s.gpuName = gpuDevices.front().vendor + " " + gpuDevices.front().name;

        double used = 0, total = 0;
        for (const auto& d : disks) {
            used += d.total - d.free;
            total += d.total;
            if (s.systemVolume.empty() && !d.name.empty() && std::toupper(static_cast<unsigned char>(d.name[0])) == 'C')
                s.systemVolume = d.name;
        }
        s.diskUsedGiB = used;
        s.diskTotalGiB = total;

        pushHistory(s);
        for (int i = 0; i < kMetricCount; ++i)
            history[i] = s.history[i];
        {
            const std::lock_guard lock(m_mutex);
            m_value = std::move(s);
        }

        while(m_running && Clock::now()-begin<std::chrono::milliseconds(m_intervalMs.load()))
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

} // namespace sysinfo
