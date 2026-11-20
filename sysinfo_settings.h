// 用户设置：常用项仍由宿主插件卡保存并通过 settingsChanged 下发；布局、文字、
// 波形、材质与交互等高级项写在插件数据目录的 advanced-settings.json。
//
// 宿主是常用项的存储方：插件改值时发 winshun/settings/update，宿主写 INI 再回
// settingsChanged（默认值 + 用户值合并后的对象）。高级项只在本地保存，首次运行
// 从宿主已有的旧值迁移。
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
    int waveOpacity = 55;
    int waveBelowOpacity = 95;
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
    int cellHeight = 40;                           // 28 – 160
    int padding = 4;
    bool dividers = true;
    int dividerOpacity = 12;
    int columns = 3;                               // 1 – 6
    int fontSize = 100;                            // 70 – 180
    QString fontFamily = QStringLiteral("Segoe UI Variable Text");
    bool tiny = false;
    bool edgeHide = false;
    bool freeHide = false;
    int freeHideDelayMs = 3000;
    bool edgeSnap = true;
    bool animations = true;
    QString animationCurve = QStringLiteral("easeOut");
    int animationDurationMs = 400;
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

// 宿主卡片声明的键（和 plugin.json 的 settings[] 一一对应，测试会核对）。
QStringList settingKeys();
QStringList hostSettingKeys();
QStringList advancedSettingKeys();
bool validWaveColors(const QString& value);

class SettingsStore {
public:
    Settings get() const;
    // 首次运行迁移宿主中已有的高级项，之后以本地文件为准。
    void initialize(const QJsonObject& merged, const QString& advancedPath);
    // 只接收宿主拥有的常用项；高级项留在本地。
    void resetHostSettings(const QJsonObject& merged);
    // 本地改动：校验、收敛，返回真正变了的键（用来发给宿主）；没变返回空对象。
    QJsonObject patch(const QJsonObject& changes);
    void preview(const QJsonObject& changes);
    QJsonObject commitPreview();
    void cancelPreview();

private:
    void saveAdvancedSettings(const Settings& settings) const;

    mutable QMutex m_mutex;
    Settings m_settings;
    QString m_advancedPath;
    Settings m_previewBase;
    bool m_previewActive = false;
};

} // namespace sysinfo
