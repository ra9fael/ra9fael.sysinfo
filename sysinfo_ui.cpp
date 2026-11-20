#include "sysinfo_ui.h"
#include "settings_window.h"

#include <QDir>
#include <QCursor>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QScreen>
#include <QStyleHints>
#include <QtGui/qscreen_platform.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <climits>

namespace sysinfo {
namespace {
constexpr int kEdgeGap = 10;

struct MonitorWork {
    QRect area;
    QString name;
};

MonitorWork monitorWork(const QRect& window)
{
    RECT bounds{window.left(), window.top(), window.left() + window.width(), window.top() + window.height()};
    const HMONITOR monitor = MonitorFromRect(&bounds, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info))
        return {};
    QString name;
    for (QScreen* screen : QGuiApplication::screens()) {
        if (const auto* native = screen->nativeInterface<QNativeInterface::QWindowsScreen>()) {
            if (native->handle() == monitor) {
                name = screen->name();
                break;
            }
        }
    }
    return {QRect(info.rcWork.left, info.rcWork.top, info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top), name};
}

QRect clampToWork(QRect rect, const QRect& area)
{
    if (!area.isValid() || rect.width() < 1 || rect.height() < 1)
        return rect;
    const int maxLeft = std::max(area.left(), area.right() - rect.width() + 1);
    const int maxTop = std::max(area.top(), area.bottom() - rect.height() + 1);
    rect.moveLeft(std::clamp(rect.left(), area.left(), maxLeft));
    rect.moveTop(std::clamp(rect.top(), area.top(), maxTop));
    return rect;
}
}

SysInfoUiBridge::SysInfoUiBridge(std::shared_ptr<SettingsStore> settings, Sampler* sampler, const QString& dataDir,
                                 QObject* parent)
    : QObject(parent), m_store(std::move(settings)), m_sampler(sampler), m_dataDir(dataDir)
{
    m_settings = m_store->get();
    loadWindowState();
    m_dark = resolveTheme() == QStringLiteral("dark");
    m_snapshotTimer.setInterval(400);
    connect(&m_snapshotTimer, &QTimer::timeout, this, &SysInfoUiBridge::refreshSnapshot);
    m_snapshotTimer.start();
}

SysInfoUiBridge::~SysInfoUiBridge()
{
    hideAll();
    delete m_settingsWindow;
    m_settingsWindow = nullptr;
}

QString SysInfoUiBridge::resolveTheme() const
{
    if (m_settings.theme != QStringLiteral("system")) return m_settings.theme;
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark ? QStringLiteral("dark")
                                                                                  : QStringLiteral("light");
}

QRect SysInfoUiBridge::availableArea() const
{
    QScreen* screen = nullptr;
    if (m_settings.displayTarget == QStringLiteral("saved") && !m_savedScreenName.isEmpty()) {
        for (QScreen* candidate : QGuiApplication::screens()) if (candidate->name() == m_savedScreenName) { screen = candidate; break; }
    }
    if (!screen && m_settings.displayTarget == QStringLiteral("primary")) screen = QGuiApplication::primaryScreen();
    if (!screen && m_overlay && m_settings.position == QStringLiteral("free")) screen = QGuiApplication::screenAt(m_overlay->geometry().center());
    if (!screen) screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) screen = QGuiApplication::primaryScreen();
    return screen ? screen->availableGeometry() : QRect(0, 0, 1280, 720);
}

void SysInfoUiBridge::applyGeometry()
{
    const qreal scale = m_settings.scale / 100.0;
    const QVector<QStringList> rows = m_settings.slotRows();
    m_gridRows = std::max(1, static_cast<int>(rows.size()));
    m_gridColumns = 1;
    for (const QStringList& row : rows) m_gridColumns = std::max(m_gridColumns, static_cast<int>(row.size()));
    m_margin = m_settings.padding * scale;
    if (m_settings.tiny) {
        m_panelWidth = m_gridColumns * m_settings.cellWidth * scale;
        m_panelHeight = m_gridRows * 28 * scale;
    } else {
        m_panelWidth = m_gridColumns * m_settings.cellWidth * scale;
        m_panelHeight = m_gridRows * m_settings.cellHeight * scale;
    }
}

void SysInfoUiBridge::placeWindow(bool resetToPreset)
{
    if (!m_overlay) return;
    const QRect area = availableArea();
    const int width = qRound(m_panelWidth + 2 * m_margin), height = qRound(m_panelHeight + 2 * m_margin);
    const int gap = qRound(kEdgeGap * m_settings.scale / 100.0);
    QPoint pos;
    if (!resetToPreset && m_settings.position == QStringLiteral("free") && m_windowPos.isValid()) pos = m_windowPos.topLeft();
    else if (m_settings.position == QStringLiteral("topLeft")) pos = {area.left() + gap, area.top() + gap};
    else if (m_settings.position == QStringLiteral("bottomLeft")) pos = {area.left() + gap, area.bottom() - height - gap + 1};
    else if (m_settings.position == QStringLiteral("bottomRight")) pos = {area.right() - width - gap + 1, area.bottom() - height - gap + 1};
    else if (m_settings.position == QStringLiteral("center")) pos = {area.center().x() - width / 2, area.center().y() - height / 2};
    else pos = {area.right() - width - gap + 1, area.top() + gap};
    QRect placed(pos, QSize(width, height));
    if (!resetToPreset && m_settings.position == QStringLiteral("free")) {
        const MonitorWork work = monitorWork(placed);
        placed = clampToWork(placed, work.area.isValid() ? work.area : area);
    } else {
        placed = clampToWork(placed, area);
    }
    m_overlay->setGeometry(placed);
    m_windowPos = m_overlay->geometry(); updateSavedScreen(); saveWindowState();
}

void SysInfoUiBridge::applyClickThrough() { if (m_overlay) m_overlay->setClickThrough(m_settings.clickThrough); }

void SysInfoUiBridge::showOverlay()
{
    if (!m_overlay) {
        m_overlay = std::make_unique<NativeOverlay>();
        m_overlay->moved = [this](const QPoint& point, const QPoint& cursor) { handleMoved(point, cursor); };
        m_overlay->edgeCollapsedChanged = [this](bool collapsed) { return setEdgeCollapsed(collapsed); };
        if (!m_overlay->create()) { m_overlay.reset(); return; }
        applyGeometry();
        placeWindow(m_settings.position != QStringLiteral("free"));
    }
    applyClickThrough(); m_overlay->setCaptureVisible(m_settings.captureVisible);
    m_overlay->setLocked(m_settings.locked); m_overlay->show(m_overlay->geometry());
    m_enabled = true; refreshSnapshot();
}

void SysInfoUiBridge::hideOverlayOnly() { if (m_overlay) m_overlay->hide(); m_enabled = false; }
void SysInfoUiBridge::hideAll() { hideOverlayOnly(); }

bool SysInfoUiBridge::showSettings()
{
    if (!m_settingsWindow) {
        m_settingsWindow = new SettingsWindow;
        m_settingsWindow->setAppearance(m_locale, resolveTheme(), m_nativeLook);
        connect(m_settingsWindow, &SettingsWindow::settingEdited, this, [this](const QJsonObject& changes) {
            const QJsonObject changed = m_store->patch(changes);
            if (!changed.isEmpty()) {
                applySettings(m_store->get());
                emit settingsEdited(changed);
            }
        });
        connect(m_settingsWindow, &SettingsWindow::previewRequested, this, [this](const QJsonObject& changes) {
            m_store->preview(changes);
            applySettings(m_store->get());
        });
        connect(m_settingsWindow, &SettingsWindow::previewCommitted, this, [this] {
            const QJsonObject changed = m_store->commitPreview();
            if (!changed.isEmpty()) {
                applySettings(m_store->get());
                emit settingsEdited(changed);
            }
        });
    }
    m_settingsWindow->setSettings(m_store->get());
    return m_settingsWindow->show();
}

void SysInfoUiBridge::applyAppearance(const QString& locale, const QString& theme, const QJsonObject& nativeLook)
{
    m_locale = locale; m_nativeLook = nativeLook; m_dark = resolveTheme() == QStringLiteral("dark");
    if (m_settingsWindow) m_settingsWindow->setAppearance(locale, theme, nativeLook);
    refreshSnapshot();
}

void SysInfoUiBridge::applySettings(const Settings& settings)
{
    const Settings previous = m_settings; m_settings = settings; m_dark = resolveTheme() == QStringLiteral("dark");
    if (m_settingsWindow) m_settingsWindow->setSettings(settings);
    if (m_overlay) {
        NativeOverlayModel model;
        model.dark=m_dark; model.edgeHide=settings.edgeHide && settings.edgeSnap; model.freeHideEnabled=settings.freeHide;
        model.freeHideDelayMs=settings.freeHideDelayMs; model.animations=settings.animations;
        model.animationCurve=settings.animationCurve==QStringLiteral("linear")?0:settings.animationCurve==QStringLiteral("easeInOut")?2:settings.animationCurve==QStringLiteral("easeIn")?3:settings.animationCurve==QStringLiteral("overshoot")?4:settings.animationCurve==QStringLiteral("sine")?5:settings.animationCurve==QStringLiteral("fastOut")?6:settings.animationCurve==QStringLiteral("spring")?7:1;
        model.animationDurationMs=settings.animationDurationMs;
        model.radius=settings.radius; model.scale=settings.scale/100.f;
        m_overlay->setModel(model);
    }
    if(m_sampler) m_sampler->setInterval(settings.intervalMs);
    const bool layoutChanged = previous.scale != settings.scale || previous.columns != settings.columns ||
                               previous.cellWidth != settings.cellWidth || previous.cellHeight != settings.cellHeight ||
                               previous.metrics != settings.metrics || previous.tiny != settings.tiny || previous.position != settings.position || previous.padding != settings.padding;
    applyGeometry();
    if (m_overlay && layoutChanged && settings.position != QStringLiteral("free"))
        placeWindow(true);
    applyClickThrough();
    if (m_overlay) m_overlay->setCaptureVisible(settings.captureVisible);
    if (!settings.edgeHide && m_edgeCollapsed) {
        m_edgeCollapsed = false;
        if (m_overlay) m_overlay->setCollapsed(false);
    }
    if (settings.edgeHide && layoutChanged && m_edgeCollapsed) {
        m_edgeCollapsed = false;
        setEdgeCollapsed(true);
    }
    if (settings.enabled) showOverlay(); else hideOverlayOnly();
    refreshSnapshot();
}

bool SysInfoUiBridge::setEdgeCollapsed(bool collapsed)
{
    if (!m_overlay || (collapsed && !m_settings.edgeHide)) return false;
    if (m_edgeCollapsed == collapsed) return collapsed;

    QRect rect = m_overlay->geometry();
    const MonitorWork work = monitorWork(rect);
    const QRect area = work.area.isValid() ? work.area : availableArea();
    if (!collapsed) {
        m_edgeCollapsed = false;
        m_overlay->setCollapsed(false);
        return true;
    }
    if (!m_settings.edgeSnap) return false;

    const int left = std::abs(rect.left() - area.left());
    const int right = std::abs(rect.right() - area.right());
    const int top = std::abs(rect.top() - area.top());
    const int bottom = std::abs(rect.bottom() - area.bottom());
    const int nearest = std::min({left, right, top, bottom});
    if (nearest > qRound(12 * m_settings.scale / 100.0)) return false;
    if (nearest == top) {
        m_edgeSide = QStringLiteral("top");
        rect.moveTop(area.top());
    } else if (nearest == bottom) {
        m_edgeSide = QStringLiteral("bottom");
        rect.moveBottom(area.bottom());
    } else if (nearest == left) {
        m_edgeSide = QStringLiteral("left");
        rect.moveLeft(area.left());
    } else {
        m_edgeSide = QStringLiteral("right");
        rect.moveRight(area.right());
    }
    const bool vertical = nearest != top && nearest != bottom;
    const int anchor = vertical ? rect.top() + rect.height() / 2
        : std::clamp(m_releaseX.value_or(rect.center().x()), rect.left(), rect.right());
    if (rect != m_overlay->geometry()) {
        m_overlay->setGeometry(rect);
        m_windowPos = rect;
        saveWindowState();
    }
    m_edgeCollapsed = true;
    m_overlay->setEdgeCollapsed(true, m_edgeSide, anchor, area);
    return true;
}

void SysInfoUiBridge::handleMoved(const QPoint& position, const QPoint& cursor)
{
    if (!m_overlay) return;
    m_releaseX = cursor.x();
    QRect rect=m_overlay->geometry(); rect.moveTopLeft(position);
    const MonitorWork work = monitorWork(rect);
    rect = clampToWork(rect, work.area.isValid() ? work.area : availableArea());
    if(m_settings.edgeSnap) {
        const QRect area=work.area.isValid()?work.area:availableArea();
        const int threshold=qRound(12*m_settings.scale/100.0);
        if(std::abs(rect.left()-area.left())<=threshold) rect.moveLeft(area.left());
        else if(std::abs(rect.right()-area.right())<=threshold) rect.moveRight(area.right());
        if(std::abs(rect.top()-area.top())<=threshold) rect.moveTop(area.top());
        else if(std::abs(rect.bottom()-area.bottom())<=threshold) rect.moveBottom(area.bottom());
    }
    m_overlay->setGeometry(rect); m_windowPos=rect; updateSavedScreen(); saveWindowState();
    if (m_settings.edgeSnap && m_settings.edgeHide) setEdgeCollapsed(true);
    if (m_settings.position != QStringLiteral("free")) {
        const QJsonObject changed=m_store->patch({{QStringLiteral("position"),QStringLiteral("free")}});
        if (!changed.isEmpty()) { m_settings=m_store->get(); emit settingsEdited(changed); }
    }
}

void SysInfoUiBridge::refreshSnapshot()
{
    if (!m_overlay || !m_sampler) return;
    const Sample sample=m_sampler->get(); const bool zh=m_locale!=QStringLiteral("en");
    NativeOverlayModel model; model.dark=m_dark; model.tiny=m_settings.tiny; model.edgeHide=m_settings.edgeHide && m_settings.edgeSnap;
    model.freeHideEnabled=m_settings.freeHide; model.freeHideDelayMs=m_settings.freeHideDelayMs;
    model.animations=m_settings.animations;
    model.animationCurve=m_settings.animationCurve==QStringLiteral("linear")?0:m_settings.animationCurve==QStringLiteral("easeInOut")?2:m_settings.animationCurve==QStringLiteral("easeIn")?3:m_settings.animationCurve==QStringLiteral("overshoot")?4:m_settings.animationCurve==QStringLiteral("sine")?5:m_settings.animationCurve==QStringLiteral("fastOut")?6:m_settings.animationCurve==QStringLiteral("spring")?7:1;
    model.animationDurationMs=m_settings.animationDurationMs;
    model.radius=m_settings.radius; model.scale=m_settings.scale/100.f;
    model.waveBackground=m_settings.wavePlacement==QStringLiteral("background") ||
        (m_settings.wavePlacement==QStringLiteral("auto") && m_settings.sparklines==QStringLiteral("wash"));
    model.waveHeight=m_settings.waveHeight;
    model.waveOpacity=model.waveBackground?m_settings.waveOpacity:m_settings.waveBelowOpacity;
    model.filledDots=m_settings.waveDots==QStringLiteral("filled");
    model.sparklines=m_settings.sparklines!=QStringLiteral("off"); model.animations=m_settings.animations;
    model.waveMode=m_settings.sparklines;
    model.glassStyle=m_settings.glassStyle==QStringLiteral("square")?2:m_settings.glassStyle==QStringLiteral("frosted")?1:0;
    model.tileSize=m_settings.tileSize;
    const auto waveColors=m_settings.waveColors.split(',');
    model.columns=m_gridColumns; model.rows=m_gridRows; model.radius=qRound((m_settings.tiny?std::min(10,m_settings.radius):m_settings.radius)*m_settings.scale/100.0);
    model.cellWidth=m_settings.cellWidth; model.cellHeight=m_settings.tiny?28:m_settings.cellHeight;
    model.blur=m_settings.blur; model.glass=m_settings.glass; model.refraction=m_settings.refraction;
    model.dispersion=m_settings.dispersion; model.rimLight=m_settings.rimLight;
    model.surfaceMotion=m_settings.surfaceMotion; model.mouseDistortion=m_settings.mouseDistortion;
    model.fontPercent=m_settings.fontSize; model.fontFamily=m_settings.fontFamily;
    model.scale=m_settings.scale/100.0f;
    model.padding=static_cast<float>(m_margin/model.scale);
    model.dividers=m_settings.dividers; model.dividerOpacity=m_settings.dividerOpacity;
    model.showLabels=m_settings.showLabels; model.showValues=m_settings.showValues; model.showUnits=m_settings.showUnits;
    model.debugPattern=qEnvironmentVariableIntValue("SYSINFO_DEBUG_PATTERN") != 0;
    int row=0;
    for(const QStringList& sourceRow:m_settings.slotRows()) { for(int col=0;col<sourceRow.size();++col) {
        const QString id=sourceRow[col]; NativeMetric metric; metric.row=row;metric.column=col;metric.empty=id.isEmpty();
        if(!metric.empty) {
            const Metric key=id==QStringLiteral("cpu")?Metric::Cpu:id==QStringLiteral("memory")?Metric::Memory:id==QStringLiteral("gpu")?Metric::Gpu:id==QStringLiteral("disk")?Metric::Disk:id==QStringLiteral("netUp")?Metric::NetUp:Metric::NetDown;
            const Reading& reading=sample.readings[static_cast<size_t>(key)]; metric.rate=key==Metric::NetUp||key==Metric::NetDown;
            metric.label=id==QStringLiteral("netUp")?(zh?QStringLiteral("上行"):QStringLiteral("Upload")):id==QStringLiteral("netDown")?(zh?QStringLiteral("下行"):QStringLiteral("Download")):id==QStringLiteral("memory")?(zh?QStringLiteral("内存"):QStringLiteral("Memory")):id.toUpper();
            if(reading.status==Status::Fresh){const MetricText text=metric.rate?formatRate(reading.value):formatPercent(reading.value);metric.value=QString::fromStdString(text.number);metric.unit=QString::fromStdString(text.unit);}
            else metric.value=reading.status==Status::Sampling?(zh?QStringLiteral("采样中"):QStringLiteral("Sampling")):QStringLiteral("—");
            metric.history=sample.history[static_cast<size_t>(key)];
            metric.color=waveColors.value(waveColors.size()==1?0:static_cast<int>(key)).trimmed();
        } model.metrics.push_back(std::move(metric));
    } ++row; }
    m_overlay->setModel(std::move(model));
}

bool SysInfoUiBridge::saveOverlaySnapshot(const QString& path) const
{
    return m_overlay && m_overlay->snapshot().save(path);
}

bool SysInfoUiBridge::saveBackdropSnapshot(const QString& path) const
{
    return m_overlay && m_overlay->backdropSnapshot().save(path);
}

bool SysInfoUiBridge::saveCapsuleSnapshot(const QString& path) const
{
    return m_overlay && m_overlay->capsuleSnapshot().save(path);
}

void SysInfoUiBridge::loadWindowState(){if(m_dataDir.isEmpty())return;QFile f(m_dataDir+QStringLiteral("/window.json"));if(!f.open(QIODevice::ReadOnly))return;const QJsonObject j=QJsonDocument::fromJson(f.readAll()).object();const int x=j.value(QStringLiteral("x")).toInt(INT_MIN),y=j.value(QStringLiteral("y")).toInt(INT_MIN);m_savedScreenName=j.value(QStringLiteral("screen")).toString();if(x!=INT_MIN&&y!=INT_MIN)m_windowPos=QRect(x,y,1,1);}
void SysInfoUiBridge::saveWindowState(){if(m_dataDir.isEmpty()||!m_windowPos.isValid())return;QDir().mkpath(m_dataDir);QFile f(m_dataDir+QStringLiteral("/window.json"));if(!f.open(QIODevice::WriteOnly|QIODevice::Truncate))return;f.write(QJsonDocument(QJsonObject{{QStringLiteral("x"),m_windowPos.x()},{QStringLiteral("y"),m_windowPos.y()},{QStringLiteral("screen"),m_savedScreenName}}).toJson(QJsonDocument::Compact));}
void SysInfoUiBridge::updateSavedScreen(){if(!m_overlay)return;const MonitorWork work=monitorWork(m_overlay->geometry());if(!work.name.isEmpty())m_savedScreenName=work.name;}

} // namespace sysinfo
