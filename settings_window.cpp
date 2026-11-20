#include "settings_window.h"

#include <QDebug>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonValue>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QUrl>
#include <QVariant>

#include <algorithm>


namespace sysinfo {
namespace {

QJsonValue toJsonValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull())
        return QJsonValue();
    if (value.typeId() == QMetaType::Bool)
        return QJsonValue(value.toBool());
    if (value.typeId() == QMetaType::QString)
        return QJsonValue(value.toString());
    if (value.canConvert<double>())
        return QJsonValue(value.toDouble());
    return QJsonValue(value.toString());
}

} // namespace

SettingsWindow::SettingsWindow(QObject* parent) : QObject(parent)
{
    m_fontFamilies = QFontDatabase::families();
}

SettingsWindow::~SettingsWindow()
{
    delete m_engine;
    m_window = nullptr;
    m_engine = nullptr;
}

void SettingsWindow::setSettings(const Settings& settings)
{
    m_settings = settings;
    m_hasSettings = true;
    rebuildValues();
    emit valuesChanged();
    emit panelMetricsChanged();
}

void SettingsWindow::setAppearance(const QString& locale, const QString& theme, const QJsonObject& nativeLook)
{
    m_locale = locale;
    m_look.clear();
    for (auto it = nativeLook.constBegin(); it != nativeLook.constEnd(); ++it)
        m_look.insert(it.key(), it.value().toVariant());
    m_look.insert(QStringLiteral("dark"), theme == QStringLiteral("dark") || nativeLook.value(QStringLiteral("dark")).toBool());
    emit appearanceChanged();
}

void SettingsWindow::rebuildValues()
{
    m_values = m_hasSettings ? QJsonValue(m_settings.toJson()).toVariant().toMap() : QVariantMap();
}

QVariantMap SettingsWindow::panelMetrics() const
{
    const QVector<QStringList> rows = m_settings.slotRows();
    int columns = 1;
    for (const QStringList& row : rows)
        columns = std::max(columns, static_cast<int>(row.size()));
    const qreal scale = m_settings.scale / 100.0;
    const qreal padding = m_settings.padding * scale;
    const int width = qRound(columns * m_settings.cellWidth * scale + 2 * padding);
    const int height = qRound(static_cast<int>(rows.size()) * m_settings.cellHeight * scale + 2 * padding);
    return QVariantMap{{QStringLiteral("width"), width},
                       {QStringLiteral("height"), height},
                       {QStringLiteral("columns"), columns},
                       {QStringLiteral("rows"), static_cast<int>(rows.size())}};
}

void SettingsWindow::ensureWindow()
{
    if (m_window)
        return;
    m_engine = new QQmlApplicationEngine(this);
    m_engine->setInitialProperties({{QStringLiteral("sysinfo"), QVariant::fromValue(this)}});
    m_engine->load(QUrl(QStringLiteral("qrc:/sysinfo/SettingsWindow.qml")));
    if (!m_engine->rootObjects().isEmpty())
        m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    if (m_window) {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        QFont iconFont(QStringLiteral("Segoe Fluent Icons"));
        if (!QFontDatabase::hasFamily(iconFont.family()))
            iconFont.setFamily(QStringLiteral("Segoe MDL2 Assets"));
        iconFont.setPixelSize(28);
        iconFont.setStyleStrategy(QFont::NoFontMerging);
        painter.setFont(iconFont);
        painter.setPen(QGuiApplication::palette().color(QPalette::WindowText));
        painter.drawText(pixmap.rect(), Qt::AlignCenter, QString(QChar(0xE950)));
        m_window->setIcon(QIcon(pixmap));
    }
    if (!m_window) {
        delete m_engine;
        m_engine = nullptr;
    }
}

bool SettingsWindow::show()
{
    ensureWindow();
    if (!m_window)
        return false;
    if (m_window->visibility() == QWindow::Minimized)
        m_window->showNormal();
    else
        m_window->show();
    if (QGuiApplication::platformName() == QStringLiteral("windows")) {
        m_window->raise();
        m_window->requestActivate();
    }
    return m_window->isVisible();
}

void SettingsWindow::closeWindow()
{
    emit previewCommitted();
    if (m_window)
        m_window->hide();
}

void SettingsWindow::edit(const QString& key, const QVariant& value)
{
    const QJsonValue converted = toJsonValue(value);
    if (!converted.isNull())
        emit settingEdited(QJsonObject{{key, converted}});
}

void SettingsWindow::preview(const QString& key, int value)
{
    emit previewRequested(QJsonObject{{key, value}});
}

void SettingsWindow::commitPreview()
{
    emit previewCommitted();
}

void SettingsWindow::applyLayout(const QVariantMap& changes)
{
    QJsonObject object;
    for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
        const QJsonValue converted = toJsonValue(it.value());
        if (!converted.isNull())
            object.insert(it.key(), converted);
    }
    if (!object.isEmpty())
        emit settingEdited(object);
}

} // namespace sysinfo
