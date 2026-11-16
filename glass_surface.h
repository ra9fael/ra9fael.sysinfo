// 把玻璃引擎接到 Qt Quick：后台线程截图+合成，主线程只换纹理。
//
// 刷新节奏照抄 TaskbarPopup：背景变了 10 ms 再看，连续未变 30 ms，30 次未变就
// 关掉“截图排除本窗口”并退到 300 ms 复核；窗口隐藏、暂停时完全停下。
#pragma once

#include <QImage>
#include <QObject>
#include <QRect>
#include <QTimer>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "glass_engine.h"

class QQuickWindow;

namespace sysinfo {

class GlassSurface : public QObject {
    Q_OBJECT
    Q_PROPERTY(QImage image READ image NOTIFY imageChanged)
    Q_PROPERTY(bool light READ light WRITE setLight NOTIFY lookChanged)
    Q_PROPERTY(int transparency READ transparency WRITE setTransparency NOTIFY lookChanged)
    Q_PROPERTY(int blur READ blur WRITE setBlur NOTIFY lookChanged)
    Q_PROPERTY(int shadow READ shadow WRITE setShadow NOTIFY lookChanged)
    Q_PROPERTY(qreal radiusDip READ radiusDip WRITE setRadiusDip NOTIFY lookChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool live READ live NOTIFY liveChanged)
public:
    explicit GlassSurface(QObject* parent = nullptr);
    ~GlassSurface() override;

    QImage image() const { return m_image; }
    bool light() const { return m_light; }
    void setLight(bool light);
    int transparency() const { return m_transparency; }
    void setTransparency(int percent);
    int blur() const { return m_blur; }
    void setBlur(int percent);
    int shadow() const { return m_shadow; }
    void setShadow(int percent);
    qreal radiusDip() const { return m_radiusDip; }
    void setRadiusDip(qreal dip);
    bool active() const { return m_active; }
    void setActive(bool active);
    bool live() const { return m_live; }

    // 面板在窗口里的位置和大小（逻辑像素，不含阴影外圈）；窗口尺寸 = 面板 + 2×margin。
    void setPanel(qreal width, qreal height, qreal margin);
    qreal windowWidth() const;
    qreal windowHeight() const;

    void attach(QQuickWindow* window);
    void detach();
    // 立刻重画一帧（显示、移动、改大小、换主题、换参数）。
    Q_INVOKABLE void refreshNow();
    // 拖动、右键菜单等：暂停后台复核，等期间沿用最后一帧。
    Q_INVOKABLE void setPaused(bool paused);

signals:
    void imageChanged();
    void lookChanged();
    void activeChanged();
    void liveChanged();
    void screenMoved();
    void frameReady(QImage image, bool changed, quint64 seq);

private:
    struct Job {
        RECT window{};
        float scale = 1; // 设备像素 / DIP，含界面缩放
        float dpr = 1;
        GlassStyle style = GlassStyle::Liquid;
        bool light = true;
        int transparency = 60;
        int blur = 55;
        int shadow = 0;
        int texW = 0, texH = 0;
        GlassRect panel{};
        float radiusPx = 20;
        bool force = false;
    };

    void startWorker();
    void workerLoop();
    void enqueue(bool force);
    void onFrame(QImage image, bool changed, quint64 seq);
    void tick();
    void pump();
    void enableExclusion(bool on);
    // 排除有时会被合成器忽略：截自己中心一小块，和最后一帧比。
    void checkExclusion();

    QQuickWindow* m_window = nullptr;
    QImage m_image;
    QRect m_panelRect;
    qreal m_margin = 28;
    qreal m_radiusDip = 20;
    int m_transparency = 60;
    int m_blur = 55;
    int m_shadow = 0;
    bool m_light = true;
    bool m_active = false;
    bool m_paused = false;
    bool m_live = true; // 截图排除是否可用（不可用就不再实时跟随背景）
    bool m_excludeDisabled = false; // SYSINFO_NO_EXCLUDE=1：从不排除（探针/截图）
    bool m_exclusionOn = false;
    bool m_exclusionChecked = false;
    bool m_pendingForce = false;
    bool m_lastFrameChanged = false;
    int m_still = 0;
    quint64 m_seq = 0;
    quint64 m_shownSeq = 0;
    QTimer m_timer;
    std::chrono::steady_clock::time_point m_settleAt{};

    // 上一帧的几何，用于判断要不要重画
    int m_lastW = 0, m_lastH = 0;
    float m_lastRadius = -1;
    GlassRect m_lastPanel{};

    // 后台线程
    std::thread m_worker;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    Job m_job;
    bool m_hasJob = false;
    bool m_stop = false;
    std::atomic<int> m_inFlight{0};

    // 工作线程私有（只在 workerLoop 里用）
    GlassEngine m_engine;
    GlassShot m_shot;
    bool m_haveShot = false;
};

} // namespace sysinfo
