// 设置窗口：宿主卡片只保留常用项，其余项在这里编辑。
//
// 窗口用 Qt Quick（宿主共享 Qt 契约保证 QtQuick、QtQuick.Controls.Basic 与
// QtQuick.Layouts），不引入 Qt Widgets。数值拖动过程中不回写，松开一次性提交；
// 布局页的改动只做预览，按“应用布局”一次落盘，关窗撤销尚未提交的预览。
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVariantMap>

#include "sysinfo_settings.h"

class QQmlApplicationEngine;
class QQuickWindow;

namespace sysinfo {

class SettingsWindow final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(QVariantMap panelMetrics READ panelMetrics NOTIFY panelMetricsChanged)
    Q_PROPERTY(QVariantMap look READ look NOTIFY appearanceChanged)
    Q_PROPERTY(QString locale READ locale NOTIFY appearanceChanged)
    Q_PROPERTY(QStringList fontFamilies READ fontFamilies CONSTANT)

public:
    explicit SettingsWindow(QObject* parent = nullptr);
    ~SettingsWindow() override;

    void setSettings(const Settings& settings);
    void setAppearance(const QString& locale, const QString& theme, const QJsonObject& nativeLook);
    bool show();

    QVariantMap values() const { return m_values; }
    QVariantMap panelMetrics() const;
    QVariantMap look() const { return m_look; }
    QString locale() const { return m_locale; }
    QStringList fontFamilies() const { return m_fontFamilies; }

    Q_INVOKABLE void edit(const QString& key, const QVariant& value);
    Q_INVOKABLE void previewLayout(const QVariantMap& changes);
    Q_INVOKABLE void cancelPreview();
    Q_INVOKABLE void applyLayout(const QVariantMap& changes);
    Q_INVOKABLE void closeWindow();

signals:
    void settingEdited(const QJsonObject& changes);
    void previewRequested(const QJsonObject& changes);
    void previewCancelled();
    void valuesChanged();
    void panelMetricsChanged();
    void appearanceChanged();

private:
    void ensureWindow();
    void rebuildValues();

    Settings m_settings;
    bool m_hasSettings = false;
    QVariantMap m_values;
    QVariantMap m_look;
    QString m_locale = QStringLiteral("zh");
    QStringList m_fontFamilies;
    QQmlApplicationEngine* m_engine = nullptr;
    QQuickWindow* m_window = nullptr;
};

} // namespace sysinfo
