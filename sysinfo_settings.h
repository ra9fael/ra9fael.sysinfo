// 用户设置：清单里声明的那些项，由宿主插件卡保存并通过 settingsChanged 下发。
//
// 宿主是唯一的存储方：插件改值时发 winshun/settings/update，宿主写 INI 再回
// settingsChanged（默认值 + 用户值合并后的对象）。这里只是那份对象的本地副本，
// 顺便做类型校验和取值范围的收敛。
#pragma once

#include <QMutex>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace sysinfo {

struct Settings {
    QString glassStyle = QStringLiteral("liquid");
    int tileSize = 24;
    QString waveColors = QStringLiteral("#36BFFA,#A78BFA,#34D399,#FBBF24,#FB7185,#60A5FA");
    bool enabled = false;
    QString sparklines = QStringLiteral("inline"); // off | inline | wash | dotted | bars
    QString wavePlacement = QStringLiteral("auto"); // auto | below | background
    int waveHeight = 0; // 0: automatic; otherwise logical pixels
    int waveOpacity = 45;
    QString waveDots = QStringLiteral("filled"); // filled | trace
    QString theme = QStringLiteral("system");      // system | light | dark
    int scale = 100;                               // 80 – 200
    int glass = 60;                                // 0 – 100
    int blur = 55;                                 // 0 – 100
    int refraction = 58;                           // 0 – 100
    int dispersion = 12;                           // 0 – 100
    int rimLight = 54;                             // 0 – 100
    int surfaceMotion = 35;                        // 0 – 100
    int shadow = 0;                                // 0 – 100
    int radius = 20;                               // 8 – 28
    QString metrics = QStringLiteral("cpu,disk,netUp,memory,gpu,netDown");
    QString position = QStringLiteral("topRight"); // topLeft | topRight | bottomLeft | bottomRight | center | free
    QString displayTarget = QStringLiteral("mouse"); // primary | mouse | saved
    int cellWidth = 120;                           // 80 – 320
    int cellHeight = 50;                           // 28 – 160
    int columns = 3;                               // 1 – 6
    int fontSize = 100;                            // 70 – 180
    QString fontFamily = QStringLiteral("Segoe UI Variable Text");
    bool tiny = false;
    bool edgeHide = false;
    bool animations = true;
    bool showLabels = true;
    bool showValues = true;
    bool showUnits = true;
    bool captureVisible = true;
    bool clickThrough = false;
    bool locked = false;
    int intervalMs = 1000; // 500 – 5000

    // 逗号分列、分号或换行分行；空项保留为空格，不自动补齐。
    QStringList slotList() const;
    QVector<QStringList> slotRows() const;
    QJsonObject toJson() const;
    static Settings fromJson(const QJsonObject& values);
};

// 清单里的键（和 plugin.json 的 settings[] 一一对应，测试会核对）。
QStringList settingKeys();

class SettingsStore {
public:
    Settings get() const;
    // 宿主的合并结果（initialize / settingsChanged）。
    void reset(const QJsonObject& merged);
    // 本地改动：校验、收敛，返回真正变了的键（用来发给宿主）；没变返回空对象。
    QJsonObject patch(const QJsonObject& changes);

private:
    mutable QMutex m_mutex;
    Settings m_settings;
};

} // namespace sysinfo
