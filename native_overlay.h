// Native Win32/D3D11 overlay. Qt remains the host/event-loop layer only;
// rendering, text and pointer interaction stay in one native pipeline.
#pragma once

#include <QString>
#include <QRect>
#include <QImage>
#include <QRect>

#include <functional>
#include <memory>
#include <vector>

struct HWND__;
using HWND = HWND__*;

namespace sysinfo {

struct NativeMetric {
    QString color;
    QString label;
    QString value;
    QString unit;
    std::vector<double> history;
    bool rate = false;
    bool empty = false;
    int row = 0;
    int column = 0;
};

struct NativeOverlayModel {
    int glassStyle=0;
    int tileSize=24;
    QString waveMode;
    std::vector<NativeMetric> metrics;
    bool dark = true;
    bool tiny = false;
    bool waveBackground = false;
    bool filledDots = true;
    int waveHeight = 0;
    int waveOpacity = 45;
    bool sparklines = true;
    bool animations = true;
    bool edgeHide = false;
    int freeHideDelayMs = 0;
    bool freeHideEnabled = false;
    bool showLabels = true;
    bool showValues = true;
    bool showUnits = true;
    bool edgeHidden = false;
    bool debugPattern = false;
    QString edgeSide = QStringLiteral("right");
    int columns = 3;
    int rows = 2;
    float padding = 20;
    bool dividers = true;
    int dividerOpacity = 12;
    float cellWidth = 120;
    float cellHeight = 50;
    int radius = 22;
    int blur = 34;
    int glass = 72;
    int refraction = 58;
    int dispersion = 12;
    int rimLight = 54;
    int surfaceMotion = 35;
    int fontPercent = 100;
    float scale = 1.0f;
    QString fontFamily;
};

class NativeOverlay {
public:
    NativeOverlay();
    ~NativeOverlay();
    NativeOverlay(const NativeOverlay&) = delete;
    NativeOverlay& operator=(const NativeOverlay&) = delete;

    bool create();
    void destroy();
    void show(const QRect& geometry);
    void hide();
    bool visible() const;
    HWND hwnd() const;
    QRect geometry() const;
    void setGeometry(const QRect& geometry);
    void setClickThrough(bool enabled);
    void setCaptureVisible(bool enabled);
    void setLocked(bool enabled);
    void setModel(NativeOverlayModel model);
    void setEdgeCollapsed(bool collapsed, const QString& side, int anchor = -1, const QRect& workArea = {});
    void setFreeCollapsed(bool collapsed);
    void setCollapsed(bool collapsed);
    bool edgeCollapsed() const;
    void render();
    QImage snapshot() const;
    QImage backdropSnapshot() const;
    QImage capsuleSnapshot() const;

    std::function<void(const QPoint&, const QPoint&)> moved;
    std::function<bool(bool)> edgeCollapsedChanged;

private:
    struct Impl;
    std::unique_ptr<Impl> m;
};

} // namespace sysinfo
