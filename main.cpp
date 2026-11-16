// ra9fael.sysinfo：悬浮在所有窗口之上的资源监视窗。
//
// 结构：GUI（Qt 事件循环）在主线程，协议（winshun.h 的 serve()）在工作线程；
// 协议线程把“建窗、改设置”排到 GUI 线程，GUI 线程的改动直接发
// winshun/settings/update（SDK 的发送带锁，任何线程都能发）。
// 原生 Win32/D3D11 Overlay 与协议/采样线程保持分离。
#include <QGuiApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaObject>
#include <QString>
#include <QTimer>

#include <QThread>

#include <chrono>
#include <memory>
#include <thread>

#include <windows.h>

#include "sysinfo_sampler.h"
#include "sysinfo_settings.h"
#include "sysinfo_ui.h"
#include "winshun.h"

namespace {

using namespace sysinfo;

constexpr wchar_t kProbeBackdropClass[] = L"SysInfoLiquidProbeBackdrop";

LRESULT CALLBACK probeBackdropProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(hwnd, &paint);
        RECT rect{}; GetClientRect(hwnd, &rect);
        HBRUSH base = CreateSolidBrush(RGB(10, 19, 34));
        FillRect(dc, &rect, base); DeleteObject(base);
        const COLORREF colors[] = {RGB(238, 70, 81), RGB(38, 214, 178), RGB(46, 139, 255), RGB(255, 193, 67)};
        for (int x = 0; x < rect.right; x += 74) {
            HBRUSH stripe = CreateSolidBrush(colors[(x / 74) % 4]);
            RECT band{x, 0, x + 30, rect.bottom}; FillRect(dc, &band, stripe); DeleteObject(stripe);
        }
        HPEN pen = CreatePen(PS_SOLID, 3, RGB(250, 252, 255));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        for (int y = 20; y < rect.bottom; y += 28) MoveToEx(dc, 0, y, nullptr), LineTo(dc, rect.right, y);
        SelectObject(dc, oldPen); DeleteObject(pen);
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255,255,255));
        HFONT font = CreateFontW(-30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HGDIOBJ oldFont = SelectObject(dc, font);
        TextOutW(dc, 90, 95, L"REFRACTION / 0123456789", 23);
        SelectObject(dc, oldFont); DeleteObject(font);
        EndPaint(hwnd, &paint);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

HWND createProbeBackdrop()
{
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{}; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = kProbeBackdropClass;
        wc.lpfnWndProc = probeBackdropProc; wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        registered = RegisterClassW(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }
    const int left=GetSystemMetrics(SM_XVIRTUALSCREEN), top=GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width=GetSystemMetrics(SM_CXVIRTUALSCREEN), height=GetSystemMetrics(SM_CYVIRTUALSCREEN);
    HWND window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kProbeBackdropClass, L"", WS_POPUP | WS_VISIBLE,
                                  left, top, width, height, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    ShowWindow(window, SW_SHOWNOACTIVATE);
    SetWindowPos(window, HWND_TOPMOST, left, top, width, height, SWP_NOACTIVATE);
    return window;
}

std::string toStdString(const QString& text)
{
    const QByteArray utf8 = text.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString toQString(const std::string& text)
{
    return QString::fromStdString(text);
}

QJsonValue toJsonValue(const winshun::Json& value);

QJsonObject toJsonObject(const winshun::Json::Object& object)
{
    QJsonObject json;
    for (const auto& [key, item] : object)
        json.insert(toQString(key), toJsonValue(item));
    return json;
}

QJsonObject toJsonObject(const winshun::Json& value)
{
    return value.isObject() ? toJsonObject(value.toObject()) : QJsonObject();
}

QJsonValue toJsonValue(const winshun::Json& value)
{
    if (value.isBool())
        return QJsonValue(value.toBool());
    if (value.isNumber())
        return QJsonValue(value.toNumber());
    if (value.isString())
        return QJsonValue(toQString(value.toString()));
    if (value.isArray()) {
        QJsonArray array;
        for (const winshun::Json& item : value.toArray())
            array.append(toJsonValue(item));
        return QJsonValue(array);
    }
    if (value.isObject())
        return QJsonValue(toJsonObject(value.toObject()));
    return QJsonValue();
}

winshun::Json toWire(const QJsonValue& value)
{
    switch (value.type()) {
    case QJsonValue::Bool:
        return winshun::Json(value.toBool());
    case QJsonValue::Double:
        return winshun::Json(value.toDouble());
    case QJsonValue::String:
        return winshun::Json(toStdString(value.toString()));
    case QJsonValue::Array: {
        winshun::Json::Array out;
        for (const QJsonValue& item : value.toArray())
            out.push_back(toWire(item));
        return winshun::Json(std::move(out));
    }
    case QJsonValue::Object: {
        winshun::Json::Object out;
        const QJsonObject object = value.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it)
            out[toStdString(it.key())] = toWire(it.value());
        return winshun::Json(std::move(out));
    }
    default:
        return winshun::Json();
    }
}

class SysInfoPlugin final : public winshun::Plugin {
public:
    std::string name() const override { return "ra9fael.sysinfo"; }

    void initialized() override
    {
        m_sampler.start();
        m_store->reset(toJsonObject(settings()));
        post([this] { createUi(); });
    }
    void shuttingDown() override
    {
        m_sampler.stop();
        post([this] { m_ui.reset(); });
        // 给 GUI 线程一点时间把窗口收掉（宿主最多等两秒左右）。
        for (int i = 0; i < 40 && m_uiAlive.load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void settingsChanged(const winshun::Json& value) override
    {
        m_store->reset(toJsonObject(value));
        post([this] {
            if (m_ui)
                m_ui->applySettings(m_store->get());
        });
    }

    void appearanceChanged() override
    {
        const std::string uiLocale = locale();
        const std::string uiTheme = theme();
        post([this, uiLocale, uiTheme] {
            if (m_ui)
                m_ui->applyAppearance(toQString(uiLocale), toQString(uiTheme), {});
        });
    }

    winshun::Items query(const winshun::Query& q) override
    {
        const bool zh = chinese();
        const Settings s = m_store->get();
        const Sample sample = m_sampler.get();
        return rows(zh, s, sample, QString::fromStdString(winshun::trim(q.text)));
    }

    winshun::Effects action(const winshun::ActionRequest& a) override
    {
        const bool zh = chinese();
        const Settings before = m_store->get();

        if (a.action == "toggleEnabled") {
            const QJsonObject changed = m_store->patch({{QStringLiteral("enabled"), !before.enabled}});
            sendSettingChange(changed);
            return {winshun::effect::replace(queryItems()), winshun::effect::keep()};
        }
        return {winshun::effect::copy(a.item["copy"].isString() ? a.item["copy"].toString() : a.item["title"].toString()),
                winshun::effect::notify(zh ? "已复制" : "Copied"),
                winshun::effect::hide()};
    }

    winshun::Json queryResult(const winshun::Query& q) override
    {
        winshun::Json result = winshun::Plugin::queryResult(q);
        result = winshun::Json(winshun::Json::Object {{"items", result.toArray()}, {"rerun", 2}});
        return result;
    }

private:
    void post(std::function<void()> fn)
    {
        QMetaObject::invokeMethod(QCoreApplication::instance(), std::move(fn), Qt::QueuedConnection);
    }

    void createUi()
    {
        m_ui = std::make_unique<SysInfoUiBridge>(m_store, &m_sampler, toQString(dataDir()));
        QObject::connect(m_ui.get(), &SysInfoUiBridge::settingsEdited, m_ui.get(),
                         [this](const QJsonObject& changed) { sendSettingChange(changed); });
        m_ui->applySettings(m_store->get());
        if (m_store->get().enabled)
            m_ui->showOverlay();
        m_uiAlive = true;
        const QStringList args=QCoreApplication::arguments();
        const int dumpAt=args.indexOf(QStringLiteral("--dump-native"));
        if(dumpAt>=0&&dumpAt+1<args.size()){
            const QString path=args.at(dumpAt+1);
            QTimer::singleShot(1500,[this,path]{if(m_ui)m_ui->saveOverlaySnapshot(path);QCoreApplication::quit();});
        }
    }

    // UI 线程的改动 → 宿主。发出去以后宿主会回 settingsChanged，本地已经先应用了。
    void sendSettingChange(const QJsonObject& changed)
    {
        if (changed.isEmpty())
            return;
        post([this] {
            if (m_ui)
                m_ui->applySettings(m_store->get());
        });
        updateHostSettings(toWire(QJsonValue(changed).toObject()));
    }

    winshun::Items queryItems()
    {
        return query(winshun::Query {});
    }

    winshun::Items rows(bool zh, const Settings& s, const Sample& sample, const QString& text)
    {
        const QString wanted = text.trimmed();
        auto matches = [&](const char* keyword) {
            return wanted.isEmpty() || wanted.compare(QString::fromUtf8(keyword), Qt::CaseInsensitive) == 0;
        };

        winshun::Items items;
        const QString overlayState = s.enabled ? (zh ? "已开启" : "on") : (zh ? "已关闭" : "off");
        if (matches("monitor") || matches("监视") || matches("显示")) {
            const QString subtitle = (zh ? QStringLiteral("液态玻璃 · %1 · %2%")
                                         : QStringLiteral("Liquid glass · %1 · %2%"))
                                         .arg(overlayState)
                                         .arg(s.scale);
            winshun::Item overlay("sysinfo:overlay", zh ? "桌面监视窗" : "Desktop monitor", toStdString(subtitle));
            overlay.withIcon("glyph:U+E7F4")
                .withScore(100)
                .withActions({{"toggleEnabled", zh ? (s.enabled ? "隐藏" : "显示") : (s.enabled ? "Hide" : "Show"), "Enter"}});
            items.push_back(std::move(overlay));
        }

        if (matches("value") || matches("数值") || matches("指标")) {
            const auto add = [&](const char* id, const QString& title, const QString& subtitle, const QString& copy) {
                winshun::Item item(id, toStdString(title), toStdString(subtitle));
                item.withCopy(toStdString(copy)).withScore(90);
                items.push_back(std::move(item));
            };
            const QString cpu = readingText(sample, Metric::Cpu, zh, false);
            const QString memory = readingText(sample, Metric::Memory, zh, false);
            const QString gpu = readingText(sample, Metric::Gpu, zh, false);
            const QString disk = readingText(sample, Metric::Disk, zh, false);
            const QString up = readingText(sample, Metric::NetUp, zh, true);
            const QString down = readingText(sample, Metric::NetDown, zh, true);
            add("sysinfo:cpu", zh ? "CPU" : "CPU", toQString(sample.processor), cpu);
            add("sysinfo:memory",
                zh ? QString("内存 %1").arg(memory) : QString("Memory %1").arg(memory),
                zh ? QString("已用 %1 / %2 GiB").arg(usedGiB(sample)).arg(totalGiB(sample))
                   : QString("Used %1 / %2 GiB").arg(usedGiB(sample)).arg(totalGiB(sample)),
                memory);
            add("sysinfo:gpu", zh ? QString("GPU %1").arg(gpu) : QString("GPU %1").arg(gpu), toQString(sample.gpuName), gpu);
            add("sysinfo:disk", zh ? QString("磁盘 %1").arg(disk) : QString("Disk %1").arg(disk),
                zh ? QString("忙时占比 · C: %1 / %2 GiB").arg(freeGiB(sample)).arg(totalGiB(sample))
                   : QString("Busy time · C: %1 / %2 GiB").arg(freeGiB(sample)).arg(totalGiB(sample)),
                disk);
            add("sysinfo:network",
                zh ? QString("网络 ↑%1 ↓%2").arg(up, down) : QString("Network ↑%1 ↓%2").arg(up, down),
                sample.netName.empty() ? QString(zh ? "全部适配器" : "All adapters") : toQString(sample.netName),
                QString("%1 / %2").arg(up, down));
        }

        if (items.empty()) {
            winshun::Item hint("sysinfo:empty", zh ? "没有匹配的项" : "No matching rows",
                               zh ? "试一试：监视窗 / 数值" : "Try: monitor / values");
            hint.withScore(10).invalid();
            items.push_back(std::move(hint));
        }
        return items;
    }

    static QString readingText(const Sample& sample, Metric metric, bool zh, bool rate)
    {
        const Reading& r = sample.readings[static_cast<size_t>(metric)];
        if (r.status == Status::Sampling)
            return zh ? "采样中" : "Sampling";
        if (r.status != Status::Fresh)
            return "—";
        if (rate) {
            const MetricText text = formatRate(r.value);
            return QString("%1 %2").arg(QString::fromStdString(text.number), QString::fromStdString(text.unit));
        }
        return QString::fromStdString(formatPercent(r.value).number) + "%";
    }

    static QString usedGiB(const Sample& sample) { return QString::number(sample.ramUsedGiB, 'f', 1); }
    static QString totalGiB(const Sample& sample) { return QString::number(sample.ramTotalGiB, 'f', 1); }
    static QString freeGiB(const Sample& sample)
    {
        return QString::number(std::max(0.0, sample.diskTotalGiB - sample.diskUsedGiB), 'f', 0);
    }

    Sampler m_sampler;
    std::shared_ptr<SettingsStore> m_store = std::make_shared<SettingsStore>();
    std::unique_ptr<SysInfoUiBridge> m_ui;
    std::atomic_bool m_uiAlive {false};
};

// 探针用的假采样器：不动任何系统 API，只造数据。Sampler 的接口是 final 的值获取，
// 这里继承后覆盖 get()。
class FakeSampler final : public Sampler {
public:
    FakeSampler()
    {
        Sample s;
        s.at = 1;
        for (int i = 0; i < kMetricCount; ++i) {
            s.readings[static_cast<size_t>(i)] = {Status::Fresh, 10.0 + i * 7};
            for (int k = 0; k < 40; ++k)
                s.history[static_cast<size_t>(i)].push_back(5.0 + i * 4 + (k % 7));
        }
        s.processor = "AMD Ryzen 9 5950X";
        s.gpuName = "NVIDIA GeForce RTX 3080";
        s.ramTotalGiB = 32;
        s.ramUsedGiB = 13.8;
        s.diskTotalGiB = 1862;
        s.diskUsedGiB = 850;
        s.netName = "Intel I225-V";
        m_fake = std::move(s);
    }

    Sample get() const override
    {
        const std::lock_guard lock(m_mutex);
        return m_fake;
    }

    void tick()
    {
        const std::lock_guard lock(m_mutex);
        for (int i = 0; i < kMetricCount; ++i) {
            auto& r = m_fake.readings[static_cast<size_t>(i)];
            r.value = std::clamp(r.value + (std::rand() % 11 - 5) * 1.7, 1.0, 95.0);
            m_fake.history[static_cast<size_t>(i)].push_back(r.value);
            if (m_fake.history[static_cast<size_t>(i)].size() > 60)
                m_fake.history[static_cast<size_t>(i)].erase(m_fake.history[static_cast<size_t>(i)].begin());
        }
        m_fake.at += 1000;
    }

private:
    mutable std::mutex m_mutex;
    Sample m_fake;
};

int runProbe(QGuiApplication& app)
{
    const bool diagnosticPattern = app.arguments().contains(QStringLiteral("--probe-pattern"));
    HWND backdrop = diagnosticPattern ? nullptr : createProbeBackdrop();
    if (diagnosticPattern)
        qputenv("SYSINFO_DEBUG_PATTERN", "1");
    FakeSampler sampler;
    auto store = std::make_shared<SettingsStore>();
    QJsonObject overrides;
    if (app.arguments().contains(QStringLiteral("--dark")))
        overrides.insert(QStringLiteral("theme"), QStringLiteral("dark"));
    if (app.arguments().contains(QStringLiteral("--light")))
        overrides.insert(QStringLiteral("theme"), QStringLiteral("light"));
    if (app.arguments().contains(QStringLiteral("--no-spark")))
        overrides.insert(QStringLiteral("sparklines"), QStringLiteral("off"));
    if (app.arguments().contains(QStringLiteral("--scale")))
        overrides.insert(QStringLiteral("scale"), 200);
    overrides.insert(QStringLiteral("position"), QStringLiteral("topLeft"));
    overrides.insert(QStringLiteral("displayTarget"), QStringLiteral("primary"));
    overrides.insert(QStringLiteral("captureVisible"), false);
    overrides.insert(QStringLiteral("blur"), 35);
    overrides.insert(QStringLiteral("glass"), 72);
    overrides.insert(QStringLiteral("refraction"), 88);
    const int settingsAt=app.arguments().indexOf(QStringLiteral("--settings-json"));
    if(settingsAt>=0 && settingsAt+1<app.arguments().size()) {
        QFile file(app.arguments().at(settingsAt+1));
        if(file.open(QIODevice::ReadOnly)) {
            const auto supplied=QJsonDocument::fromJson(file.readAll()).object();
            for(auto it=supplied.begin();it!=supplied.end();++it) overrides.insert(it.key(),it.value());
        }
    }
    store->patch(overrides);
    store->patch({{QStringLiteral("enabled"), true}});

    const int dataAt=app.arguments().indexOf(QStringLiteral("--probe-data"));
    const QString probeData=dataAt>=0 && dataAt+1<app.arguments().size()?app.arguments().at(dataAt+1):QString();
    SysInfoUiBridge bridge(store, &sampler, probeData);
    QObject::connect(&bridge, &SysInfoUiBridge::settingsEdited, &bridge,
                     [&store, &bridge](const QJsonObject& changed) {
                         store->patch(changed);
                         bridge.applySettings(store->get());
                     });
    bridge.applySettings(store->get());
    bridge.showOverlay();

    QTimer ticker;
    QObject::connect(&ticker, &QTimer::timeout, [&sampler, &bridge] {
        sampler.tick();
        bridge.refreshSnapshot();
    });
    ticker.start(1000);

    const QStringList args=app.arguments();
    const int dumpAt=args.indexOf(QStringLiteral("--dump-native"));
    if(dumpAt>=0&&dumpAt+1<args.size()){
        const QString path=args.at(dumpAt+1);
        QTimer::singleShot(1800,[&bridge,path,backdrop]{
            bridge.saveOverlaySnapshot(path);
            bridge.saveBackdropSnapshot(path + QStringLiteral(".backdrop.png"));
            if (backdrop) DestroyWindow(backdrop);QCoreApplication::quit();
        });
    }

    return app.exec();
}

} // namespace

int main(int argc, char* argv[])
{
    if(argc>1 && std::strcmp(argv[1],"--verify-sampling")==0) {
        Sampler sampler;
        sampler.setInterval(500); sampler.start();
        for(int interval:{500,1000,5000,500}) {
            sampler.setInterval(interval);
            auto previous=sampler.get().at;
            int observed=0;
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(25);
            while(observed<3 && std::chrono::steady_clock::now()<deadline) {
                const auto current=sampler.get().at;
                if(current!=previous) {
                    if(observed>0) {
                        const auto delta=current-previous;
                        std::printf("sampling target=%d actual=%llu ms\n",interval,static_cast<unsigned long long>(delta));
                        if(delta<static_cast<unsigned>(interval-60) || delta>static_cast<unsigned>(interval+300)) return 2;
                    }
                    previous=current; ++observed;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if(observed<3) return 3;
        }
        sampler.stop(); return 0;
    }
    // 探针模式：sysinfo.exe --probe 不接宿主，直接开监视窗 + 假数据，
    // 用来验证玻璃渲染和截屏排除；正常启动仍然走插件协议。
    const bool probe = argc > 1 && std::strcmp(argv[1], "--probe") == 0;

    // Qt only hosts the plugin event loop; the overlay is native Win32/D3D11.
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setOrganizationName(QStringLiteral("WinShun"));
    app.setApplicationName(QStringLiteral("ra9fael.sysinfo"));

    if (probe)
        return runProbe(app);

    SysInfoPlugin plugin;
    int rc = 0;
    std::thread serve([&] {
        rc = plugin.serve();
        QMetaObject::invokeMethod(&app, &QCoreApplication::quit, Qt::QueuedConnection);
    });
    app.exec();
    serve.join();
    return rc;
}
