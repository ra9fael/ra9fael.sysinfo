import QtQuick
import QtTest

Item {
    id: fixture
    width: 800
    height: 700

    QtObject {
        id: sysinfo
        property string locale: "en"
        property var look: ({ dark: false })
        property var fontFamilies: ["Segoe UI", "Consolas"]
        property var values: ({
            intervalMs: 1000, scale: 100, freeHideDelayMs: 3000,
            metrics: "cpu,disk,netUp;memory,gpu,netDown", columns: 3,
            cellWidth: 120, cellHeight: 40, padding: 4, dividerOpacity: 12,
            fontSize: 100, fontFamily: "Segoe UI", waveHeight: 0,
            sparklines: "inline", wavePlacement: "auto", waveDots: "filled",
            waveOpacity: 55, waveBelowOpacity: 95, waveColors: "#36BFFA",
            theme: "system", glassStyle: "liquid", glass: 60, blur: 55,
            refraction: 58, dispersion: 12, rimLight: 54, tileSize: 24,
            radius: 20, displayTarget: "primary", position: "free", surfaceMotion: 35
        })
        signal appearanceChanged()
        onLookChanged: appearanceChanged()
        function commitPreview() {}
    }

    TestCase {
        name: "SettingsDropdown"
        when: windowShown

        function test_metricOptions_data() {
            return [{ tag: "light", dark: false }, { tag: "dark", dark: true }]
        }

        function test_metricOptions(data) {
            sysinfo.look = { dark: data.dark }
            const component = Qt.createComponent("../SettingsWindow.qml")
            compare(component.status, Component.Ready, component.errorString())
            const window = component.createObject(null, { visible: true })
            verify(!!window, "Object exists")
            try {
                const tabs = findChild(window, "tabs")
                const metrics = findChild(window, "metricCombos")
                verify(!!tabs && !!metrics, "Object exists")
                tabs.currentIndex = 1
                const combo = metrics.itemAt(0)
                verify(!!combo, "Object exists")
                tryCompare(combo, "visible", true)
                combo.focus = true
                mouseClick(combo)
                tryCompare(combo.popup, "opened", true)
                const list = combo.popup.contentItem
                tryVerify(() => !!list.itemAtIndex(2))
                const cpu = list.itemAtIndex(1).contentItem
                tryCompare(cpu, "text", qsTr("CPU"))
                tryCompare(cpu, "visible", true)
                verify(cpu.width > 0 && cpu.height > 0)
                const memory = list.itemAtIndex(2)
                compare(memory.contentItem.text, qsTr("Memory"))
                mouseClick(memory)
                tryCompare(combo, "currentText", qsTr("Memory"))
                tryCompare(combo.popup, "opened", false)
            } finally {
                window.destroy()
            }
        }
    }
}
