#include "sysinfo_settings.h"

#include <QSet>
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
                       {QStringLiteral("waveDots"), waveDots},
                       {QStringLiteral("theme"), theme},
                        {QStringLiteral("scale"), scale},
                        {QStringLiteral("glass"), glass},
                        {QStringLiteral("blur"), blur},
                        {QStringLiteral("refraction"), refraction},
                        {QStringLiteral("dispersion"), dispersion},
                        {QStringLiteral("rimLight"), rimLight},
                        {QStringLiteral("surfaceMotion"), surfaceMotion},
                        {QStringLiteral("shadow"), shadow},
                        {QStringLiteral("radius"), radius},
                        {QStringLiteral("metrics"), metrics},
                        {QStringLiteral("position"), position},
                        {QStringLiteral("displayTarget"), displayTarget},
                        {QStringLiteral("cellWidth"), cellWidth},
                        {QStringLiteral("cellHeight"), cellHeight},
                        {QStringLiteral("columns"), columns},
                        {QStringLiteral("fontSize"), fontSize},
                        {QStringLiteral("fontFamily"), fontFamily},
                        {QStringLiteral("tiny"), tiny},
                        {QStringLiteral("edgeHide"), edgeHide},
                        {QStringLiteral("animations"), animations},
                        {QStringLiteral("showLabels"), showLabels},
                        {QStringLiteral("showValues"), showValues},
                        {QStringLiteral("showUnits"), showUnits},
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
    if(v.value(QStringLiteral("waveColors")).isString()) s.waveColors=v.value(QStringLiteral("waveColors")).toString();
    s.wavePlacement = choice(v.value(QStringLiteral("wavePlacement")),
        {QStringLiteral("auto"), QStringLiteral("below"), QStringLiteral("background")}, s.wavePlacement);
    s.waveHeight = clampInt(v.value(QStringLiteral("waveHeight")), s.waveHeight, 0, 136);
    if (s.waveHeight > 0) s.waveHeight = std::max(4, s.waveHeight);
    s.waveOpacity = clampInt(v.value(QStringLiteral("waveOpacity")), s.waveOpacity, 0, 100);
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
    if (v.contains(QStringLiteral("fontSize")))
        s.fontSize = clampInt(v.value(QStringLiteral("fontSize")), s.fontSize, 70, 180);
    if (v.contains(QStringLiteral("fontFamily")) && v.value(QStringLiteral("fontFamily")).isString())
        s.fontFamily = v.value(QStringLiteral("fontFamily")).toString().trimmed();
    if (s.fontFamily.isEmpty())
        s.fontFamily = QStringLiteral("Segoe UI Variable Text");
    if (v.contains(QStringLiteral("tiny")))
        s.tiny = v.value(QStringLiteral("tiny")).toBool(s.tiny);
    if (v.contains(QStringLiteral("edgeHide")))
        s.edgeHide = v.value(QStringLiteral("edgeHide")).toBool(s.edgeHide);
    if (v.contains(QStringLiteral("animations")))
        s.animations = v.value(QStringLiteral("animations")).toBool(s.animations);
    if (v.contains(QStringLiteral("showLabels")))
        s.showLabels = v.value(QStringLiteral("showLabels")).toBool(s.showLabels);
    if (v.contains(QStringLiteral("showValues")))
        s.showValues = v.value(QStringLiteral("showValues")).toBool(s.showValues);
    if (v.contains(QStringLiteral("showUnits")))
        s.showUnits = v.value(QStringLiteral("showUnits")).toBool(s.showUnits);
    if (v.contains(QStringLiteral("captureVisible")))
        s.captureVisible = v.value(QStringLiteral("captureVisible")).toBool(s.captureVisible);
    if (v.contains(QStringLiteral("clickThrough")))
        s.clickThrough = v.value(QStringLiteral("clickThrough")).toBool(s.clickThrough);
    if (v.contains(QStringLiteral("locked")))
        s.locked = v.value(QStringLiteral("locked")).toBool(s.locked);
    if (v.contains(QStringLiteral("intervalMs")))
        s.intervalMs = clampInt(v.value(QStringLiteral("intervalMs")), s.intervalMs, 500, 5000);
    return s;
}

QStringList settingKeys()
{
    static const QStringList keys = {
        QStringLiteral("enabled"),    QStringLiteral("sparklines"),
        QStringLiteral("wavePlacement"), QStringLiteral("waveHeight"), QStringLiteral("waveOpacity"), QStringLiteral("waveDots"),
        QStringLiteral("waveColors"), QStringLiteral("theme"),
        QStringLiteral("scale"),      QStringLiteral("glass"), QStringLiteral("glassStyle"), QStringLiteral("tileSize"), QStringLiteral("blur"), QStringLiteral("refraction"),
        QStringLiteral("dispersion"), QStringLiteral("rimLight"), QStringLiteral("surfaceMotion"),
        QStringLiteral("shadow"),     QStringLiteral("radius"),     QStringLiteral("metrics"),
        QStringLiteral("columns"), QStringLiteral("cellWidth"), QStringLiteral("cellHeight"),
        QStringLiteral("fontSize"), QStringLiteral("fontFamily"), QStringLiteral("tiny"),
        QStringLiteral("position"), QStringLiteral("displayTarget"), QStringLiteral("edgeHide"),
        QStringLiteral("animations"), QStringLiteral("showLabels"), QStringLiteral("showValues"),
        QStringLiteral("showUnits"), QStringLiteral("captureVisible"),
        QStringLiteral("clickThrough"), QStringLiteral("locked"),  QStringLiteral("intervalMs")};
    return keys;
}

Settings SettingsStore::get() const
{
    const QMutexLocker lock(&m_mutex);
    return m_settings;
}

void SettingsStore::reset(const QJsonObject& merged)
{
    const Settings parsed = Settings::fromJson(merged);
    const QMutexLocker lock(&m_mutex);
    m_settings = parsed;
}

QJsonObject SettingsStore::patch(const QJsonObject& changes)
{
    const Settings before = get();
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
        const QMutexLocker lock(&m_mutex);
        m_settings = after;
    }
    return changed;
}

} // namespace sysinfo
