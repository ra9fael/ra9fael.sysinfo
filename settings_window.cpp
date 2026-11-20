#include "settings_window.h"

#include <QDebug>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonValue>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QUrl>
#include <QVariant>

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
    delete m_window;
    m_window = nullptr;
}

void SettingsWindow::setSettings(const Settings& settings)
{
    m_settings = settings;
    m_hasSettings = true;
    rebuildValues();
    emit valuesChanged();
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

void SettingsWindow::ensureWindow()
{
    if (m_window)
        return;
    m_engine = new QQmlEngine(this);
    m_engine->rootContext()->setContextProperty(QStringLiteral("sysinfo"), this);
    QQmlComponent component(m_engine, QUrl(QStringLiteral("qrc:/sysinfo/SettingsWindow.qml")));
    QObject* object = component.create();
    m_window = qobject_cast<QQuickWindow*>(object);
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
        qWarning().noquote() << "sysinfo: settings window failed:" << component.errorString();
        delete object;
    }
}

void SettingsWindow::show()
{
    ensureWindow();
    if (!m_window)
        return;
    m_window->show();
    m_window->raise();
    m_window->requestActivate();
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

void SettingsWindow::applyLayout(const QString& metrics, int columns)
{
    emit settingEdited(QJsonObject{{QStringLiteral("columns"), columns},
                                  {QStringLiteral("metrics"), metrics}});
}

} // namespace sysinfo
