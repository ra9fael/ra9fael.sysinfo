#include "sysinfo_settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QColor>
#include <QRegularExpression>

#include <algorithm>

namespace sysinfo {
namespace {

QStringList knownMetrics()
{
    return {QStringLiteral("cpu"), QStringLiteral("memory"), QStringLiteral("gpu"), QStringLiteral("disk"),
            QStringLiteral("netUp"), QStringLiteral("netDown")};
}

const QStringList& knownPositions()
{
    static const QStringList positions = {QStringLiteral("topLeft"), QStringLiteral("topRight"),
                                          QStringLiteral("bottomLeft"), QStringLiteral("bottomRight"),
                                          QStringLiteral("center"), QStringLiteral("free")};
    return positions;
}

int clampInt(const QJsonValue& value, int fallback, int low, int high)
{
    if (!value.isDouble())
        return fallback;
    return std::clamp(static_cast<int>(std::lround(value.toDouble())), low, high);
}

QString choice(const QJsonValue& value, const QStringList& allowed, const QString& fallback)
{
    if (!value.isString())
        return fallback;
    const QString text = value.toString();
    return allowed.contains(text) ? text : fallback;
}

} // namespace

QStringList Settings::slotList() const
{
    QStringList out;
    for (const QStringList& row : slotRows())
        out.append(row);
    while (!out.isEmpty() && out.back().isEmpty())
        out.removeLast();
    if (out.isEmpty())
        out.append(QStringLiteral("cpu"));
    return out;
}

QVector<QStringList> Settings::slotRows() const
{
    QVector<QStringList> rows;
    const QStringList known = knownMetrics();
    const QStringList sourceRows = metrics.split(QRegularExpression(QStringLiteral("[;\\n]")), Qt::KeepEmptyParts);
    for (const QString& source : sourceRows) {
        QStringList row;
        for (const QString& raw : source.split(QLatin1Char(','), Qt::KeepEmptyParts)) {
            const QString id = raw.trimmed();
            if (id.isEmpty())
                row.append(QString());
            else if (known.contains(id))
                row.append(id);
        }
        while (!row.isEmpty() && row.back().isEmpty())
            row.removeLast();
        if (!row.isEmpty())
            rows.append(std::move(row));
    }
    if (rows.isEmpty())
        rows.append({QStringLiteral("cpu")});
    // Without an explicit row separator, columns determine automatic wrapping.
    if (!metrics.contains(QLatin1Char(';')) && !metrics.contains(QLatin1Char('\n'))) {
        const QStringList flat = rows.front();
        rows.clear();
        for (qsizetype start = 0; start < flat.size(); start += columns)
            rows.append(flat.mid(start, columns));
    }
    return rows;
}

QJsonObject Settings::toJson() const
{
    return QJsonObject{{QStringLiteral("enabled"), enabled},
                       {QStringLiteral("waveColors"), waveColors},
                       {QStringLiteral("glassStyle"), glassStyle},
                       {QStringLiteral("tileSize"), tileSize},
                       {QStringLiteral("sparklines"), sparklines},
                       {QStringLiteral("wavePlacement"), wavePlacement},
                       {QStringLiteral("waveHeight"), waveHeight},
                       {QStringLiteral("waveOpacity"), waveOpacity},
                       {QStringLiteral("waveBelowOpacity"), waveBelowOpacity},
                       {QStringLiteral("waveDots"), waveDots},
                       {QStringLiteral("theme"), theme},
                        {QStringLiteral("scale"), scale},
                        {QStringLiteral("glass"), glass},
                        {QStringLiteral("blur"), blur},
                        {QStringLiteral("refraction"), refraction},
                        {QStringLiteral("dispersion"), dispersion},
                        {QStringLiteral("rimLight"), rimLight},
                        {QStringLiteral("surfaceMotion"), surfaceMotion},
                        {QStringLiteral("mouseDistortion"), mouseDistortion},
                        {QStringLiteral("shadow"), shadow},
                        {QStringLiteral("radius"), radius},
                        {QStringLiteral("metrics"), metrics},
                        {QStringLiteral("position"), position},
                        {QStringLiteral("displayTarget"), displayTarget},
                        {QStringLiteral("cellWidth"), cellWidth},
                        {QStringLiteral("cellHeight"), cellHeight},
                        {QStringLiteral("padding"), padding},
                        {QStringLiteral("dividers"), dividers},
                        {QStringLiteral("dividerOpacity"), dividerOpacity},
                        {QStringLiteral("columns"), columns},
                        {QStringLiteral("fontSize"), fontSize},
                        {QStringLiteral("fontFamily"), fontFamily},
                        {QStringLiteral("tiny"), tiny},
                         {QStringLiteral("edgeHide"), edgeHide},
                         {QStringLiteral("freeHide"), freeHide},
                         {QStringLiteral("freeHideDelayMs"), freeHideDelayMs},
                         {QStringLiteral("edgeSnap"), edgeSnap},
                        {QStringLiteral("animations"), animations},
                        {QStringLiteral("animationCurve"), animationCurve},
                        {QStringLiteral("animationDurationMs"), animationDurationMs},
                        {QStringLiteral("showLabels"), showLabels},
                        {QStringLiteral("showValues"), showValues},
                        {QStringLiteral("showUnits"), showUnits},
                        {QStringLiteral("labelLanguage"), labelLanguage},
                        {QStringLiteral("captureVisible"), captureVisible},
                       {QStringLiteral("clickThrough"), clickThrough},
                       {QStringLiteral("locked"), locked},
                       {QStringLiteral("intervalMs"), intervalMs}};
}

Settings Settings::fromJson(const QJsonObject& v)
{
    Settings s;
    s.glassStyle=choice(v.value(QStringLiteral("glassStyle")),{QStringLiteral("liquid"),QStringLiteral("frosted"),QStringLiteral("square")},s.glassStyle);
    s.tileSize=clampInt(v.value(QStringLiteral("tileSize")),s.tileSize,8,80);
    if (v.contains(QStringLiteral("enabled")))
        s.enabled = v.value(QStringLiteral("enabled")).toBool(s.enabled);
    s.sparklines = choice(v.value(QStringLiteral("sparklines")),
                          {QStringLiteral("off"), QStringLiteral("inline"), QStringLiteral("wash"), QStringLiteral("dotted"), QStringLiteral("bars")}, s.sparklines);
    if (v.value(QStringLiteral("waveColors")).isString() && validWaveColors(v.value(QStringLiteral("waveColors")).toString()))
        s.waveColors = v.value(QStringLiteral("waveColors")).toString();
    s.wavePlacement = choice(v.value(QStringLiteral("wavePlacement")),
        {QStringLiteral("auto"), QStringLiteral("below"), QStringLiteral("background")}, s.wavePlacement);
    s.waveHeight = clampInt(v.value(QStringLiteral("waveHeight")), s.waveHeight, 0, 136);
    if (s.waveHeight > 0) s.waveHeight = std::max(4, s.waveHeight);
    s.waveOpacity = clampInt(v.value(QStringLiteral("waveOpacity")), s.waveOpacity, 0, 100);
    s.waveBelowOpacity = clampInt(v.value(QStringLiteral("waveBelowOpacity")), s.waveBelowOpacity, 0, 100);
    s.waveDots = choice(v.value(QStringLiteral("waveDots")),
        {QStringLiteral("filled"), QStringLiteral("trace")}, s.waveDots);
    s.theme = choice(v.value(QStringLiteral("theme")),
                     {QStringLiteral("system"), QStringLiteral("light"), QStringLiteral("dark")}, s.theme);
    if (v.contains(QStringLiteral("scale")))
        s.scale = clampInt(v.value(QStringLiteral("scale")), s.scale, 80, 200);
    if (v.contains(QStringLiteral("glass")))
        s.glass = clampInt(v.value(QStringLiteral("glass")), s.glass, 0, 100);
    if (v.contains(QStringLiteral("blur")))
        s.blur = clampInt(v.value(QStringLiteral("blur")), s.blur, 0, 100);
    if (v.contains(QStringLiteral("refraction")))
        s.refraction = clampInt(v.value(QStringLiteral("refraction")), s.refraction, 0, 100);
    if (v.contains(QStringLiteral("dispersion")))
        s.dispersion = clampInt(v.value(QStringLiteral("dispersion")), s.dispersion, 0, 100);
    if (v.contains(QStringLiteral("rimLight")))
        s.rimLight = clampInt(v.value(QStringLiteral("rimLight")), s.rimLight, 0, 100);
    if (v.contains(QStringLiteral("surfaceMotion")))
        s.surfaceMotion = clampInt(v.value(QStringLiteral("surfaceMotion")), s.surfaceMotion, 0, 100);
    if (v.contains(QStringLiteral("mouseDistortion")))
        s.mouseDistortion = v.value(QStringLiteral("mouseDistortion")).toBool(s.mouseDistortion);
    if (v.contains(QStringLiteral("shadow")))
        s.shadow = clampInt(v.value(QStringLiteral("shadow")), s.shadow, 0, 100);
    if (v.contains(QStringLiteral("radius")))
        s.radius = clampInt(v.value(QStringLiteral("radius")), s.radius, 8, 28);
    if (v.contains(QStringLiteral("metrics")) && v.value(QStringLiteral("metrics")).isString()) {
        const QString raw = v.value(QStringLiteral("metrics")).toString();
        s.metrics = raw;
    }
    s.position = choice(v.value(QStringLiteral("position")), knownPositions(), s.position);
    s.displayTarget = choice(v.value(QStringLiteral("displayTarget")),
                             {QStringLiteral("primary"), QStringLiteral("mouse"), QStringLiteral("saved")},
                             s.displayTarget);
    if (v.contains(QStringLiteral("cellWidth")))
        s.cellWidth = clampInt(v.value(QStringLiteral("cellWidth")), s.cellWidth, 80, 320);
    if (v.contains(QStringLiteral("cellHeight")))
        s.cellHeight = clampInt(v.value(QStringLiteral("cellHeight")), s.cellHeight, 28, 160);
    if (v.contains(QStringLiteral("columns")))
        s.columns = clampInt(v.value(QStringLiteral("columns")), s.columns, 1, 6);
    s.padding = clampInt(v.value(QStringLiteral("padding")), s.padding, 0, 40);
    s.dividers = v.value(QStringLiteral("dividers")).toBool(s.dividers);
    s.dividerOpacity = clampInt(v.value(QStringLiteral("dividerOpacity")), s.dividerOpacity, 0, 100);
    if (v.contains(QStringLiteral("fontSize")))
        s.fontSize = clampInt(v.value(QStringLiteral("fontSize")), s.fontSize, 70, 180);
    if (v.contains(QStringLiteral("fontFamily")) && v.value(QStringLiteral("fontFamily")).isString())
        s.fontFamily = v.value(QStringLiteral("fontFamily")).toString().trimmed();
    if (s.fontFamily.isEmpty())
        s.fontFamily = kDefaultFontFamily;
    if (v.contains(QStringLiteral("tiny")))
        s.tiny = v.value(QStringLiteral("tiny")).toBool(s.tiny);
    if (v.contains(QStringLiteral("edgeHide")))
        s.edgeHide = v.value(QStringLiteral("edgeHide")).toBool(s.edgeHide);
    if (v.contains(QStringLiteral("freeHide")))
        s.freeHide = v.value(QStringLiteral("freeHide")).toBool(s.freeHide);
    if (v.contains(QStringLiteral("freeHideDelayMs")))
        s.freeHideDelayMs = clampInt(v.value(QStringLiteral("freeHideDelayMs")),s.freeHideDelayMs,500,30000);
    s.edgeSnap = v.value(QStringLiteral("edgeSnap")).toBool(s.edgeSnap);
    if (v.contains(QStringLiteral("animations")))
        s.animations = v.value(QStringLiteral("animations")).toBool(s.animations);
    s.animationCurve = choice(v.value(QStringLiteral("animationCurve")),
        {QStringLiteral("linear"), QStringLiteral("easeOut"), QStringLiteral("easeInOut"), QStringLiteral("easeIn"),
         QStringLiteral("overshoot"), QStringLiteral("sine"), QStringLiteral("fastOut"), QStringLiteral("spring")}, s.animationCurve);
    s.animationDurationMs = clampInt(v.value(QStringLiteral("animationDurationMs")), s.animationDurationMs, 100, 1500);
    if (v.contains(QStringLiteral("showLabels")))
        s.showLabels = v.value(QStringLiteral("showLabels")).toBool(s.showLabels);
    if (v.contains(QStringLiteral("showValues")))
        s.showValues = v.value(QStringLiteral("showValues")).toBool(s.showValues);
    if (v.contains(QStringLiteral("showUnits")))
        s.showUnits = v.value(QStringLiteral("showUnits")).toBool(s.showUnits);
    s.labelLanguage = choice(v.value(QStringLiteral("labelLanguage")),
                             {QStringLiteral("follow"), QStringLiteral("zh"), QStringLiteral("en")}, s.labelLanguage);
    if (v.contains(QStringLiteral("captureVisible")))
        s.captureVisible = v.value(QStringLiteral("captureVisible")).toBool(s.captureVisible);
    if (v.contains(QStringLiteral("clickThrough")))
        s.clickThrough = v.value(QStringLiteral("clickThrough")).toBool(s.clickThrough);
    if (v.contains(QStringLiteral("locked")))
        s.locked = v.value(QStringLiteral("locked")).toBool(s.locked);
    if (v.contains(QStringLiteral("intervalMs")))
        s.intervalMs = clampInt(v.value(QStringLiteral("intervalMs")), s.intervalMs, 100, 5000);
    return s;
}

QStringList settingKeys()
{
    return hostSettingKeys();
}

QStringList hostSettingKeys()
{
    return {QStringLiteral("enabled"), QStringLiteral("intervalMs"), QStringLiteral("tiny"),
            QStringLiteral("scale"), QStringLiteral("locked"), QStringLiteral("edgeHide"),
            QStringLiteral("freeHide"), QStringLiteral("freeHideDelayMs"),
            QStringLiteral("clickThrough")};
}

QStringList advancedSettingKeys()
{
    static const QStringList keys = {
        QStringLiteral("metrics"), QStringLiteral("columns"), QStringLiteral("cellWidth"),
        QStringLiteral("cellHeight"), QStringLiteral("padding"), QStringLiteral("dividers"),
        QStringLiteral("dividerOpacity"), QStringLiteral("fontFamily"), QStringLiteral("fontSize"),
        QStringLiteral("showLabels"), QStringLiteral("showValues"), QStringLiteral("showUnits"),
        QStringLiteral("labelLanguage"),
        QStringLiteral("sparklines"), QStringLiteral("wavePlacement"), QStringLiteral("waveHeight"),
        QStringLiteral("waveDots"), QStringLiteral("waveColors"), QStringLiteral("waveOpacity"),
        QStringLiteral("waveBelowOpacity"), QStringLiteral("theme"), QStringLiteral("glassStyle"),
        QStringLiteral("glass"), QStringLiteral("blur"), QStringLiteral("refraction"),
        QStringLiteral("dispersion"), QStringLiteral("rimLight"), QStringLiteral("tileSize"),
        QStringLiteral("radius"), QStringLiteral("shadow"), QStringLiteral("displayTarget"),
        QStringLiteral("position"), QStringLiteral("edgeSnap"), QStringLiteral("edgeHide"),
        QStringLiteral("freeHide"), QStringLiteral("freeHideDelayMs"), QStringLiteral("animations"),
        QStringLiteral("animationCurve"), QStringLiteral("animationDurationMs"),
        QStringLiteral("surfaceMotion"), QStringLiteral("mouseDistortion"), QStringLiteral("captureVisible")};
    return keys;
}

bool validWaveColors(const QString& value)
{
    const QStringList colors = value.split(QLatin1Char(','), Qt::KeepEmptyParts);
    if (colors.size() != 1 && colors.size() != 6)
        return false;
    static const QRegularExpression hex(QStringLiteral("^#[0-9A-Fa-f]{6}$"));
    return std::all_of(colors.cbegin(), colors.cend(), [&](const QString& color) {
        return hex.match(color.trimmed()).hasMatch() && QColor(color.trimmed()).isValid();
    });
}

Settings SettingsStore::get() const
{
    const QMutexLocker lock(&m_mutex);
    return m_settings;
}

QJsonObject SettingsStore::patch(const QJsonObject& changes)
{
    const QMutexLocker lock(&m_mutex);
    const Settings before = m_settings;
    QJsonObject merged = before.toJson();
    for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
        if (merged.contains(it.key()) && merged.value(it.key()).type() == it.value().type())
            merged.insert(it.key(), it.value());
    }
    const Settings after = Settings::fromJson(merged);
    const QJsonObject normalized = after.toJson();
    const QJsonObject oldJson = before.toJson();

    QJsonObject changed;
    for (auto it = normalized.constBegin(); it != normalized.constEnd(); ++it) {
        if (it.value() != oldJson.value(it.key()))
            changed.insert(it.key(), it.value());
    }
    if (!changed.isEmpty()) {
        m_settings = after;
        bool advancedChanged = false;
        for (auto it = changed.constBegin(); it != changed.constEnd() && !advancedChanged; ++it)
            advancedChanged = advancedSettingKeys().contains(it.key());
        if (advancedChanged && !m_previewActive)
            saveAdvancedSettings(m_settings);
    }
    return changed;
}

void SettingsStore::initialize(const QJsonObject& merged, const QString& advancedPath)
{
    m_advancedPath = advancedPath;
    QJsonObject migrated = merged;
    const QJsonObject defaults = Settings().toJson();
    for (const QString& key : advancedSettingKeys()) {
        const QJsonValue value = migrated.value(key);
        if (value.isString() && defaults.value(key).isDouble()) {
            bool ok = false;
            const double number = value.toString().toDouble(&ok);
            if (ok)
                migrated.insert(key, number);
        } else if (value.isString() && defaults.value(key).isBool()) {
            const QString text = value.toString();
            migrated.insert(key, text.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 || text == QStringLiteral("1"));
        }
    }
    Settings initial = Settings::fromJson(migrated);
    QFile file(m_advancedPath);
    bool loaded = false;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonParseError error{};
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error == QJsonParseError::NoError && document.isObject()) {
            const QJsonObject saved = document.object();
            QJsonObject combined = initial.toJson();
            for (const QString& key : advancedSettingKeys()) {
                if (saved.contains(key))
                    combined.insert(key, saved.value(key));
            }
            initial = Settings::fromJson(combined);
            loaded = true;
        }
        file.close();
    }
    if (!loaded)
        saveAdvancedSettings(initial);
    const QMutexLocker lock(&m_mutex);
    m_settings = initial;
    m_previewActive = false;
}

void SettingsStore::resetHostSettings(const QJsonObject& merged)
{
    const QMutexLocker lock(&m_mutex);
    QJsonObject combined = m_settings.toJson();
    for (const QString& key : hostSettingKeys()) {
        if (merged.contains(key))
            combined.insert(key, merged.value(key));
    }
    m_settings = Settings::fromJson(combined);
}

void SettingsStore::preview(const QJsonObject& changes)
{
    const QMutexLocker lock(&m_mutex);
    if (!m_previewActive) {
        m_previewBase = m_settings;
        m_previewActive = true;
    }
    QJsonObject merged = m_settings.toJson();
    for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
        if (merged.contains(it.key()) && merged.value(it.key()).type() == it.value().type())
            merged.insert(it.key(), it.value());
    }
    m_settings = Settings::fromJson(merged);
}

QJsonObject SettingsStore::commitPreview()
{
    const QMutexLocker lock(&m_mutex);
    if (!m_previewActive)
        return {};
    const QJsonObject before = m_previewBase.toJson();
    const QJsonObject after = m_settings.toJson();
    QJsonObject changed;
    for (auto it = after.constBegin(); it != after.constEnd(); ++it) {
        if (it.value() != before.value(it.key()))
            changed.insert(it.key(), it.value());
    }
    bool advancedChanged = false;
    for (auto it = changed.constBegin(); it != changed.constEnd() && !advancedChanged; ++it)
        advancedChanged = advancedSettingKeys().contains(it.key());
    if (advancedChanged)
        saveAdvancedSettings(m_settings);
    m_previewActive = false;
    return changed;
}

void SettingsStore::cancelPreview()
{
    const QMutexLocker lock(&m_mutex);
    if (!m_previewActive)
        return;
    m_settings = m_previewBase;
    m_previewActive = false;
}

void SettingsStore::saveAdvancedSettings(const Settings& settings) const
{
    if (m_advancedPath.isEmpty())
        return;
    const QFileInfo info(m_advancedPath);
    QDir().mkpath(info.absolutePath());
    QJsonObject advanced;
    const QJsonObject all = settings.toJson();
    for (const QString& key : advancedSettingKeys())
        advanced.insert(key, all.value(key));
    QSaveFile file(m_advancedPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(advanced).toJson(QJsonDocument::Compact));
        file.commit();
    }
}

} // namespace sysinfo
