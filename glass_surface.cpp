#include "glass_surface.h"

#include <QDebug>
#include <QQuickWindow>
#include <QScreen>

#include <chrono>
#include <cstring>
#include <algorithm>

#include <dwmapi.h>
#include <windows.h>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

namespace sysinfo {
namespace {

using Clock = std::chrono::steady_clock;

// TaskbarPopup 的节奏：打开排除后等合成器生效，再截。
constexpr int kExcludeSettleMs = 50;
constexpr int kStillTicks = 30;

RECT windowRect(HWND hwnd)
{
    RECT r{};
    GetWindowRect(hwnd, &r);
    return r;
}

} // namespace

GlassSurface::GlassSurface(QObject* parent)
    : QObject(parent)
{
    // 探针/截图用：跳过“排除本窗口”，让普通截屏也能看到玻璃。
    m_excludeDisabled = qEnvironmentVariable("SYSINFO_NO_EXCLUDE") == QLatin1String("1");
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &GlassSurface::tick);
    connect(this, &GlassSurface::frameReady, this, &GlassSurface::onFrame, Qt::QueuedConnection);
}

GlassSurface::~GlassSurface()
{
    {
        const std::lock_guard lock(m_mutex);
        m_stop = true;
    }
    m_cv.notify_all();
    if (m_worker.joinable())
        m_worker.join();
}

void GlassSurface::setLight(bool light)
{
    if (m_light == light)
        return;
    m_light = light;
    emit lookChanged();
    refreshNow();
}

void GlassSurface::setTransparency(int percent)
{
    percent = std::clamp(percent, 0, 100);
    if (m_transparency == percent)
        return;
    m_transparency = percent;
    emit lookChanged();
    refreshNow();
}

void GlassSurface::setBlur(int percent)
{
    percent = std::clamp(percent, 0, 100);
    if (m_blur == percent)
        return;
    m_blur = percent;
    emit lookChanged();
    refreshNow();
}

void GlassSurface::setShadow(int percent)
{
    percent = std::clamp(percent, 0, 100);
    if (m_shadow == percent)
        return;
    m_shadow = percent;
    emit lookChanged();
    refreshNow();
}

void GlassSurface::setRadiusDip(qreal dip)
{
    if (qFuzzyCompare(m_radiusDip, dip))
        return;
    m_radiusDip = dip;
    emit lookChanged();
    refreshNow();
}

void GlassSurface::setPanel(qreal width, qreal height, qreal margin)
{
    m_panelRect = QRect(0, 0, qRound(width), qRound(height));
    m_margin = margin;
}

qreal GlassSurface::windowWidth() const
{
    return m_panelRect.width() + 2 * m_margin;
}

qreal GlassSurface::windowHeight() const
{
    return m_panelRect.height() + 2 * m_margin;
}

void GlassSurface::attach(QQuickWindow* window)
{
    if (m_window == window)
        return;
    detach();
    m_window = window;
    startWorker();
    if (!window)
        return;
    connect(window, &QWindow::visibleChanged, this, [this](bool visible) {
        setActive(visible);
    });
    connect(window, &QWindow::widthChanged, this, &GlassSurface::refreshNow);
    connect(window, &QWindow::heightChanged, this, &GlassSurface::refreshNow);
    connect(window, &QWindow::xChanged, this, &GlassSurface::refreshNow);
    connect(window, &QWindow::yChanged, this, &GlassSurface::refreshNow);
    connect(window, &QWindow::screenChanged, this, [this](QScreen*) {
        if (m_active)
            refreshNow();
        emit screenMoved();
    });
    if (window->isVisible())
        setActive(true);
}

void GlassSurface::detach()
{
    setActive(false);
    if (m_window)
        disconnect(m_window, nullptr, this, nullptr);
    m_window = nullptr;
}
void GlassSurface::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    emit activeChanged();
    if (!active) {
        m_timer.stop();
        enableExclusion(false);
        return;
    }
    m_still = 0;
    refreshNow();
}

void GlassSurface::setPaused(bool paused)
{
    m_paused = paused;
    if (paused)
        m_timer.stop();
    else
        refreshNow();
}

void GlassSurface::refreshNow()
{
    if (!m_active || !m_window)
        return;
    m_still = 0;
    m_pendingForce = true;
    pump();
}

void GlassSurface::startWorker()
{
    if (m_worker.joinable())
        return;
    m_worker = std::thread([this] { workerLoop(); });
}

void GlassSurface::enqueue(bool force)
{
    if (!m_window)
        return;
    Job job;
    const HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    job.window = windowRect(hwnd);
    job.dpr = m_window->devicePixelRatio();
    job.scale = static_cast<float>(job.dpr);
    job.style = GlassStyle::Liquid;
    job.light = m_light;
    job.transparency = m_transparency;
    job.blur = m_blur;
    job.shadow = m_shadow;
    job.texW = job.window.right - job.window.left;
    job.texH = job.window.bottom - job.window.top;
    const qreal dpr = job.dpr;
    const qreal margin = m_margin * dpr;
    job.panel = GlassRect{static_cast<float>(margin), static_cast<float>(margin),
                          static_cast<float>(m_panelRect.width() * dpr),
                          static_cast<float>(m_panelRect.height() * dpr)};
    job.radiusPx = static_cast<float>(m_radiusDip * dpr);
    job.force = force;
    if (job.texW <= 0 || job.texH <= 0)
        return;
    {
        const std::lock_guard lock(m_mutex);
        if (m_stop)
            return;
        m_job = job;
        m_hasJob = true;
    }
    ++m_inFlight;
    m_cv.notify_one();
}

void GlassSurface::workerLoop()
{
    for (;;) {
        Job job;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this] { return m_stop || m_hasJob; });
            if (m_stop)
                return;
            job = m_job;
            m_hasJob = false;
        }

        m_engine.setStyle(job.style);
        m_engine.setLight(job.light);
        m_engine.setTransparency(job.transparency);
        m_engine.setBlur(job.blur);
        m_engine.setShadow(job.shadow);

        GlassShot shot;
        const bool changed = GlassEngine::shoot(job.window, job.scale, job.style, shot, m_haveShot ? &m_shot : nullptr);
        const bool resize = job.texW != m_lastW || job.texH != m_lastH || job.radiusPx != m_lastRadius ||
                            job.panel.x != m_lastPanel.x || job.panel.y != m_lastPanel.y ||
                            job.panel.w != m_lastPanel.w || job.panel.h != m_lastPanel.h;

        QImage image;
        if (changed || job.force || resize) {
            if (changed) {
                GlassShot copy{shot.area, shot.w, shot.h, shot.raw, shot.blur};
                m_engine.adopt(std::move(copy), job.scale);
                m_shot = std::move(shot);
                m_haveShot = true;
            }
            if (m_haveShot) {
                m_engine.prepare(job.texW, job.texH, job.panel, job.radiusPx);
                std::vector<uint32_t> pixels(static_cast<size_t>(job.texW) * job.texH, 0u);
                const POINT origin{job.window.left, job.window.top};
                m_engine.render(pixels.data(), origin);
                image = QImage(job.texW, job.texH, QImage::Format_ARGB32_Premultiplied);
                for (int y = 0; y < job.texH; ++y)
                    std::memcpy(image.scanLine(y), pixels.data() + static_cast<size_t>(y) * job.texW,
                                static_cast<size_t>(job.texW) * 4);
                image.setDevicePixelRatio(job.dpr);
                m_lastW = job.texW;
                m_lastH = job.texH;
                m_lastRadius = job.radiusPx;
                m_lastPanel = job.panel;
            }
        }
        emit frameReady(image, changed, ++m_seq);
    }
}

void GlassSurface::onFrame(QImage image, bool changed, quint64 seq)
{
    --m_inFlight;
    if (!m_active)
        return;
    m_lastFrameChanged = changed;    if (!image.isNull() && (changed || m_image.isNull() || seq != m_shownSeq)) {
        m_image = std::move(image);
        m_shownSeq = seq;
        emit imageChanged();
        if (!m_exclusionChecked) {
            m_exclusionChecked = true;
            QTimer::singleShot(80, this, [this] { checkExclusion(); });
        }
    }
    if (changed) {
        m_still = 0;
    } else {
        ++m_still;
        if (m_still >= kStillTicks && m_exclusionOn) {
            enableExclusion(false);
            m_still = 0; // 下一轮重新打开排除再复核
        }
    }
    // The overlay owns a CPU-composited image and Qt Quick creates a texture
    // for each delivered frame. Continuously recapturing a static desktop
    // grows renderer pressure in the resident host without improving the
    // monitor's content. Refresh on visible state, movement, display changes
    // and setting changes instead; those paths all call refreshNow().
    if (m_pendingForce)
        pump();
}

void GlassSurface::tick()
{
    pump();
}

void GlassSurface::pump()
{
    if (!m_active || !m_window || m_paused)
        return;
    if (m_inFlight.load() > 0)
        return;
    if (m_excludeDisabled) {
        const bool force = m_pendingForce;
        m_pendingForce = false;
        enqueue(force);
        return;
    }
    const auto now = Clock::now();
    if (!m_exclusionOn) {
        enableExclusion(true);
        m_settleAt = now + std::chrono::milliseconds(kExcludeSettleMs);
        m_timer.start(kExcludeSettleMs);
        return;
    }
    const auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(m_settleAt - now).count();
    if (wait > 0) {
        m_timer.start(static_cast<int>(wait) + 1);
        return;
    }
    const bool force = m_pendingForce;
    m_pendingForce = false;
    enqueue(force);
}

void GlassSurface::enableExclusion(bool on)
{
    if (!m_window)
        return;
    const HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    if (!hwnd)
        return;
    if (on == m_exclusionOn)
        return;
    if (on) {
        if (!SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE)) {
            // 系统不支持（Win10 2004 以前）：玻璃只在显示、移动和改参数时刷新。
            m_live = false;
            emit liveChanged();
            return;
        }
        m_exclusionOn = true;
    } else {
        SetWindowDisplayAffinity(hwnd, WDA_NONE);
        m_exclusionOn = false;
    }
}

// 排除有时会被合成器忽略：截自己中心一小块，和最后一帧比，几乎一样说明截到了自己。
void GlassSurface::checkExclusion()
{
    if (!m_window || m_image.isNull())
        return;
    const HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    RECT wr = windowRect(hwnd);
    const qreal dpr = m_window->devicePixelRatio();
    const int size = qRound(24 * dpr);
    const int cx = wr.left + qRound((m_margin + m_panelRect.width() / 2.0) * dpr);
    const int cy = wr.top + qRound((m_margin + m_panelRect.height() / 2.0) * dpr);
    RECT patch{cx - size / 2, cy - size / 2, cx + size / 2, cy + size / 2};
    std::vector<uint32_t> px;
    // 直接借玻璃引擎的截图逻辑：给它一个“窗口”，取回中间这块。
    GlassShot shot;
    if (!GlassEngine::shoot(patch, 1.0f, GlassStyle::Liquid, shot) || shot.raw.empty())
        return;
    const int imgX = qRound((m_margin + m_panelRect.width() / 2.0) * dpr) - size / 2;
    const int imgY = qRound((m_margin + m_panelRect.height() / 2.0) * dpr) - size / 2;
    const int ox = patch.left - shot.area.left;
    const int oy = patch.top - shot.area.top;
    long long diff = 0;
    int n = 0;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const int sx = imgX + x, sy = imgY + y;
            const int px = ox + x, py = oy + y;
            if (sx < 0 || sy < 0 || sx >= m_image.width() || sy >= m_image.height())
                continue;
            if (px < 0 || py < 0 || px >= shot.w || py >= shot.h)
                continue;
            const uint32_t a = m_image.pixel(sx, sy);
            const uint32_t b = shot.raw[static_cast<size_t>(py) * shot.w + px];
            diff += std::abs(static_cast<int>((a >> 16) & 0xFF) - static_cast<int>((b >> 16) & 0xFF));
            diff += std::abs(static_cast<int>((a >> 8) & 0xFF) - static_cast<int>((b >> 8) & 0xFF));
            diff += std::abs(static_cast<int>(a & 0xFF) - static_cast<int>(b & 0xFF));
            n += 3;
        }
    }
    if (n == 0)
        return;
    const double mean = static_cast<double>(diff) / n;
    if (mean < 8.0) {
        m_live = false;
        emit liveChanged();
        qWarning("sysinfo: capture exclusion does not work; the glass follows only on show/move");
    }
}

} // namespace sysinfo
