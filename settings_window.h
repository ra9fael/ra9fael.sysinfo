// 设置窗口：宿主卡片只保留常用项，其余项在这里编辑。
//
// 窗口用 Qt Quick（宿主共享 Qt 契约保证 QtQuick、QtQuick.Controls.Basic 与
// QtQuick.Layouts），不引入 Qt Widgets。数值拖动时预览，松开提交；单项改动
// 立即生效；布局改动按“应用布局”一次提交。
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVariantMap>

#include "sysinfo_settings.h"

class QQmlEngine;
class QQuickWindow;

namespace sysinfo {

class SettingsWindow final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(QVariantMap look READ look NOTIFY appearanceChanged)
    Q_PROPERTY(QString locale READ locale NOTIFY appearanceChanged)
    Q_PROPERTY(QStringList fontFamilies READ fontFamilies CONSTANT)

public:
    explicit SettingsWindow(QObject* parent = nullptr);
    ~SettingsWindow() override;

    void setSettings(const Settings& settings);
    void setAppearance(const QString& locale, const QString& theme, const QJsonObject& nativeLook);
    void show();

    QVariantMap values() const { return m_values; }
    QVariantMap look() const { return m_look; }
    QString locale() const { return m_locale; }
    QStringList fontFamilies() const { return m_fontFamilies; }

    Q_INVOKABLE void edit(const QString& key, const QVariant& value);
    Q_INVOKABLE void preview(const QString& key, int value);
    Q_INVOKABLE void commitPreview();
    Q_INVOKABLE void applyLayout(const QString& metrics, int columns);
    Q_INVOKABLE void closeWindow();

signals:
    void settingEdited(const QJsonObject& changes);
    void previewRequested(const QJsonObject& changes);
    void previewCommitted();
    void valuesChanged();
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
    QQmlEngine* m_engine = nullptr;
    QQuickWindow* m_window = nullptr;
};

} // namespace sysinfo
