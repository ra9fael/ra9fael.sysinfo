// Native overlay bridge: host protocol and settings remain Qt-based, while the
// visual surface is a Win32/D3D11 window with DirectWrite content.
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QRect>
#include <QTimer>

#include <memory>

#include "native_overlay.h"
#include "sysinfo_sampler.h"
#include "sysinfo_settings.h"

namespace sysinfo {

class SettingsWindow;

class SysInfoUiBridge : public QObject {
    Q_OBJECT
public:
    SysInfoUiBridge(std::shared_ptr<SettingsStore> settings, Sampler* sampler, const QString& dataDir,
                    QObject* parent = nullptr);
    ~SysInfoUiBridge() override;

    void applyAppearance(const QString& locale, const QString& theme, const QJsonObject& nativeLook);
    void applySettings(const Settings& settings);
    void showOverlay();
    HWND overlayHandle() const { return m_overlay ? m_overlay->hwnd() : nullptr; }
    bool showSettings();
    void hideAll();
    void refreshSnapshot();
    bool saveOverlaySnapshot(const QString& path) const;
    bool saveBackdropSnapshot(const QString& path) const;
    bool saveCapsuleSnapshot(const QString& path) const;

signals:
    void settingsEdited(const QJsonObject& changed);

private:
    void applyGeometry();
    void placeWindow(bool resetToPreset);
    void hideOverlayOnly();
    void loadWindowState();
    void saveWindowState();
    void updateSavedScreen();
    void applyClickThrough();
    QRect availableArea() const;
    QString resolveTheme() const;
    void handleMoved(const QPoint& position, const QPoint& cursor);
    bool setEdgeCollapsed(bool collapsed);

    std::shared_ptr<SettingsStore> m_store;
    Sampler* m_sampler = nullptr;
    QString m_dataDir;
    std::unique_ptr<NativeOverlay> m_overlay;
    SettingsWindow* m_settingsWindow = nullptr;
    Settings m_settings;
    bool m_enabled = false;
    bool m_dark = true;
    QString m_locale = QStringLiteral("zh");
    QJsonObject m_nativeLook;
    QRect m_windowPos;
    QString m_savedScreenName;
    bool m_edgeCollapsed = false;
    QString m_edgeSide = QStringLiteral("right");
    std::optional<int> m_releaseX;
    qreal m_margin = 20;
    qreal m_panelWidth = 360;
    qreal m_panelHeight = 100;
    int m_gridColumns = 3;
    int m_gridRows = 2;
    QTimer m_snapshotTimer;
};

} // namespace sysinfo
