// 设置模型和玻璃参数的纯逻辑测试（不碰任何系统 API）。
#include "../glass_engine.h"
#include "../sysinfo_core.h"
#include "../sysinfo_settings.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>

#include <cassert>
#include <cmath>
#include <iostream>

using namespace sysinfo;

static void testCore()
{
    const auto first = rate(std::nullopt, 100, 1000);
    assert(first.status == Status::Sampling);

    const auto normal = rate(Counter {100, 1000}, 350, 2000);
    assert(normal.status == Status::Fresh);
    assert(std::abs(normal.perSecond - 250.0) < 0.001);

    const auto reset = rate(Counter {500, 1000}, 20, 2000);
    assert(reset.status == Status::Sampling);

    const auto staleSample = SamplePoint {12.0, 1000, Status::Fresh};
    assert(statusAt(staleSample, 2000, 1000) == Status::Fresh);
    assert(statusAt(staleSample, 2001, 1000) == Status::Stale);

    assert(formatPercent(11.4).number == "11");
    assert(formatPercent(9.84).number == "9.8");
    assert(formatPercent(9.84).unit == "%");
    assert(formatRate(299110.0).unit == "KB/s");
    assert(std::abs(std::stod(formatRate(299110.0).number) - 292.1) < 0.05);
    assert(formatRate(2.0 * 1024 * 1024).unit == "MB/s");
    assert(formatRate(512.0).unit == "B/s");
    assert(metricText(formatPercent(11)) == "11 %");

    std::cout << "sysinfo_core: all checks passed\n";
}

static void testSettings()
{
    // 清单里的键和设置模型一一对应
    const QStringList keys = settingKeys();
    assert(keys.size() == 9);
    const QStringList advanced = advancedSettingKeys();
    assert(advanced.size() == 37);
    assert(advanced.contains("waveColors") && advanced.contains("glassStyle") && advanced.contains("tileSize"));
    assert(keys.contains("enabled") && keys.contains("intervalMs") && keys.contains("tiny")
            && keys.contains("scale") && keys.contains("locked") && keys.contains("edgeHide")
            && keys.contains("freeHide") && keys.contains("freeHideDelayMs") && keys.contains("clickThrough"));
    QFile manifest(QStringLiteral(SYSINFO_SOURCE_DIR "/plugin.json"));
    assert(manifest.open(QIODevice::ReadOnly));
    const QJsonArray declared = QJsonDocument::fromJson(manifest.readAll()).object().value("settings").toArray();
    QStringList declaredKeys;
    QJsonObject freeHideDelay;
    for (const QJsonValue& setting : declared) {
        const QJsonObject object = setting.toObject();
        declaredKeys.append(object.value("key").toString());
        if (object.value("key").toString() == "freeHideDelayMs")
            freeHideDelay = object;
        assert(!object.value("title").toObject().value("zh").toString().startsWith("基本"));
        assert(!object.value("title").toObject().value("en").toString().startsWith("General"));
    }
    assert(declaredKeys == keys);
    assert(freeHideDelay.value("min").toInt() == 500 && freeHideDelay.value("max").toInt() == 30000);
    assert(Settings::fromJson({{"freeHideDelayMs", 50000}}).freeHideDelayMs == 30000);

    // 默认值
    SettingsStore store;
    const Settings defaults = store.get();
    assert(!defaults.enabled);
    assert(defaults.sparklines == "inline");
    assert(defaults.wavePlacement == "auto" && defaults.waveHeight == 0);
    assert(defaults.waveOpacity == 55 && defaults.waveBelowOpacity == 95 && defaults.waveDots == "filled");
    assert(defaults.scale == 100);
    assert(defaults.glass == 60);
    assert(defaults.blur == 55 && defaults.refraction == 58 && defaults.dispersion == 12
           && defaults.rimLight == 54 && defaults.surfaceMotion == 35 && defaults.shadow == 0);
    assert(defaults.showLabels && defaults.showValues && defaults.showUnits && defaults.captureVisible);
    assert(defaults.columns == 3 && defaults.cellWidth == 120 && defaults.cellHeight == 40);
    assert(defaults.padding == 4 && defaults.dividers && defaults.dividerOpacity == 12);
    assert(defaults.edgeSnap && !defaults.edgeHide);
    const Settings independentEdges=Settings::fromJson({{"edgeSnap", false}, {"edgeHide", true}});
    assert(!independentEdges.edgeSnap && independentEdges.edgeHide);
    assert(defaults.radius == 20);
    const QStringList slotList = defaults.slotList();
    assert(slotList.size() == 6);
    assert(slotList[0] == "cpu" && slotList[1] == "disk" && slotList[2] == "netUp");
    assert(slotList[3] == "memory" && slotList[4] == "gpu" && slotList[5] == "netDown");

    // 宿主的合并结果
    store.resetHostSettings(defaults.toJson());
    assert(store.get().sparklines == "inline");

    // 改动：值收敛、返回变化子集
    const QJsonObject changed = store.patch({{"scale", 250}, {"radius", 4}, {"sparklines", "wash"}, {"enabled", true}});
    assert(changed.contains("scale") && changed.value("scale").toDouble() == 200); // 越界收敛到上限
    assert(changed.contains("radius") && changed.value("radius").toDouble() == 8);
    assert(changed.contains("sparklines") && changed.value("sparklines").toString() == "wash");
    assert(changed.contains("enabled") && changed.value("enabled").toBool() == true);
    assert(!changed.contains("glass"));
    assert(store.get().scale == 200 && store.get().enabled);
    assert(store.get().sparklines == "wash");

    // 相同值不发
    for (const QString& shape : {QStringLiteral("inline"), QStringLiteral("wash"), QStringLiteral("dotted"), QStringLiteral("bars")}) {
        for (const QString& placement : {QStringLiteral("below"), QStringLiteral("background")}) {
            SettingsStore waves;
            waves.patch({{"sparklines", shape}, {"wavePlacement", placement}, {"waveHeight", 16},
                         {"waveOpacity", 65}, {"waveDots", "trace"}});
            const Settings roundTrip = Settings::fromJson(waves.get().toJson());
            assert(roundTrip.sparklines == shape && roundTrip.wavePlacement == placement);
            assert(roundTrip.waveHeight == 16 && roundTrip.waveOpacity == 65 && roundTrip.waveDots == "trace");
            waves.patch({{"waveHeight", 0}});
            assert(waves.get().waveHeight == 0);
        }
    }
    const Settings bounds = Settings::fromJson({{"waveHeight", 999}, {"waveOpacity", -1}});
    assert(bounds.waveHeight == 136 && bounds.waveOpacity == 0);
    assert(Settings::fromJson({{"waveHeight", 1}}).waveHeight == 4);
    assert(store.patch({{"scale", 200}}).isEmpty());

    // 未知键、类型不对被忽略
    assert(store.patch({{"nope", true}}).isEmpty());
    assert(store.patch({{"scale", "wide"}}).isEmpty());

    // 波形颜色只接受一个或六个 #RRGGBB
    assert(validWaveColors("#123456"));
    assert(validWaveColors("#123456,#654321,#abcdef,#fedcba,#012345,#6789ab"));
    assert(!validWaveColors("#123456,#654321"));
    assert(!validWaveColors("red"));
    assert(Settings::fromJson({{"waveColors", "#GGGGGG"}}).waveColors == Settings().waveColors);

    // metrics：忽略未知项，空项保持为空格且不再补齐。
    {
        SettingsStore probe;
        const QJsonObject r = probe.patch({{"metrics", "gpu,, netDown;nope"}});
        assert(r.contains("metrics"));
        const QStringList slots2 = probe.get().slotList();
        assert(slots2.size() == 3);
        assert(slots2[0] == "gpu" && slots2[1].isEmpty() && slots2[2] == "netDown");
    }

    {
        SettingsStore probe;
        const QJsonObject r = probe.patch({{"columns", 0}, {"blur", 120}, {"refraction", -2}, {"shadow", -2}, {"fontSize", 200}});
        assert(r.value("columns").toDouble() == 1);
        assert(r.value("blur").toDouble() == 100);
        assert(r.value("refraction").toDouble() == 0);
        assert(r.value("shadow").toDouble() == 0);
        assert(r.value("fontSize").toDouble() == 180);
    }

    // 槽位换位
    {
        Settings probe;
        probe.metrics = "cpu,disk,netUp,memory,gpu,netDown";
        const QStringList slotNames = probe.slotList();
        assert(slotNames.join(',') == "cpu,disk,netUp,memory,gpu,netDown");
    }

    {
        Settings probe;
        probe.columns = 3;
        probe.metrics = "cpu,,netUp;memory,gpu,netDown";
        const QVector<QStringList> rows = probe.slotRows();
        assert(rows.size() == 2);
        assert(rows[0].size() == 3 && rows[0][0] == "cpu" && rows[0][1].isEmpty() && rows[0][2] == "netUp");
        assert(rows[1].join(',') == "memory,gpu,netDown");
    }
}

static void testSettingsStorage()
{
    QTemporaryDir dir;
    assert(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("advanced-settings.json"));

    // 首次运行：宿主发来的旧字符串值换成正确类型，高级值落到本地文件
    {
        SettingsStore store;
        store.initialize({{"fontSize", "150"}, {"edgeSnap", "false"}, {"sparklines", "wash"},
                          {"theme", "dark"}, {"enabled", true}}, path);
        const Settings s = store.get();
        assert(s.fontSize == 150 && !s.edgeSnap && s.sparklines == "wash" && s.theme == "dark");
        assert(s.enabled);
        QFile file(path);
        assert(file.open(QIODevice::ReadOnly));
        const QJsonObject saved = QJsonDocument::fromJson(file.readAll()).object();
        assert(saved.value("sparklines").toString() == "wash");
        assert(!saved.contains("enabled"));
    }

    // 再次启动：本地文件优先，宿主键仍来自宿主
    {
        SettingsStore store;
        store.initialize({{"sparklines", "bars"}, {"enabled", false}}, path);
        assert(store.get().sparklines == "wash");
        assert(!store.get().enabled);
    }

    // 高级值改动立即写盘；宿主键改动只留在内存
    {
        SettingsStore store;
        store.initialize({}, path);
        store.patch({{"blur", 12}, {"enabled", true}});
        QFile file(path);
        assert(file.open(QIODevice::ReadOnly));
        const QJsonObject saved = QJsonDocument::fromJson(file.readAll()).object();
        assert(saved.value("blur").toInt() == 12);

        store.resetHostSettings({{"enabled", true}, {"scale", 150}, {"locked", true}});
        assert(store.get().enabled && store.get().scale == 150 && store.get().locked);
        assert(store.get().blur == 12);
    }

    // 拖动预览只在提交时落盘
    {
        SettingsStore store;
        store.initialize({}, path);
        auto savedBlur = [&]() {
            QFile file(path);
            assert(file.open(QIODevice::ReadOnly));
            return QJsonDocument::fromJson(file.readAll()).object().value("blur").toInt();
        };
        store.preview({{"blur", 30}});
        assert(store.get().blur == 30);
        assert(savedBlur() == 12);
        const QJsonObject committed = store.commitPreview();
        assert(committed.contains("blur") && committed.value("blur").toInt() == 30);
        assert(!store.commitPreview().contains("blur"));
        assert(savedBlur() == 30);
    }

    // 取消预览回到原值
    {
        SettingsStore store;
        store.initialize({}, path);
        store.preview({{"blur", 70}});
        assert(store.get().blur == 70);
        store.cancelPreview();
        assert(store.get().blur == 30);
    }

    // 无法解析的文件按缺失处理并重写
    {
        const QString broken = dir.filePath(QStringLiteral("broken.json"));
        QFile file(broken);
        assert(file.open(QIODevice::WriteOnly));
        file.write("{");
        file.close();
        SettingsStore store;
        store.initialize({{"sparklines", "bars"}}, broken);
        assert(store.get().sparklines == "bars");
        QFile check(broken);
        assert(check.open(QIODevice::ReadOnly));
        assert(QJsonDocument::fromJson(check.readAll()).object().value("sparklines").toString() == "bars");
    }
}

static void testGlassMath()
{
    // 玻璃引擎的纯数学部分：直边上距离是负的深度，角上更浅
    float nx = 0, ny = 0;
    const float dEdge = roundRectDistance(0.0f, 10.0f, 50.0f, 20.0f, 6.0f, nx, ny);
    const float dCorner = roundRectDistance(48.0f, 18.0f, 50.0f, 20.0f, 6.0f, nx, ny);
    assert(dEdge == -10.0f); // 直边上：往里 10px
    assert(dCorner > -6.0f && dCorner < 0.0f);
}

int main()
{
    testCore();
    testSettings();
    testSettingsStorage();
    testGlassMath();
    std::cout << "sysinfo tests: all passed\n";
    return 0;
}
