import QtQuick
import QtTest

Item {
    id: fixture
    width: 900
    height: 1100

    // Mirrors the contract SettingsWindow expects from SysInfoUiBridge and records
    // every call, so tests can tell previews from committed edits.
    QtObject {
        id: sysinfo
        property string locale: "zh"
        property var look: ({ dark: false })
        property var fontFamilies: ["Segoe UI", "Consolas"]
        property var panelMetrics: ({ width: 368, height: 88, columns: 3, rows: 2 })
        property var values: ({})
        property var edits: []
        property var previews: []
        property var layouts: []
        property int cancels: 0

        signal appearanceChanged()
        onLookChanged: appearanceChanged()

        function reset() {
            edits = []
            previews = []
            layouts = []
            cancels = 0
        }
        function edit(key, value) { edits = edits.concat([{ key: key, value: value }]) }
        function previewLayout(changes) { previews = previews.concat([changes]) }
        function applyLayout(changes) { layouts = layouts.concat([changes]) }
        function cancelPreview() { cancels += 1 }
        function closeWindow() {}
    }

    TestCase {
        name: "SettingsWindow"
        when: windowShown

        readonly property var baseValues: ({
            enabled: true,
            intervalMs: 1000, scale: 100, freeHideDelayMs: 3000,
            metrics: "cpu,disk,netUp;memory,gpu,netDown", columns: 3,
            cellWidth: 120, cellHeight: 40, padding: 4, dividers: true, dividerOpacity: 12,
            tiny: false, labelLanguage: "follow",
            showLabels: true, showValues: true, showUnits: true,
            fontSize: 100, fontFamily: "Segoe UI", waveHeight: 0,
            sparklines: "inline", wavePlacement: "auto", waveDots: "filled",
            waveOpacity: 55, waveBelowOpacity: 95, waveColors: "#36BFFA",
            theme: "system", glassStyle: "liquid", glass: 60, blur: 55,
            refraction: 58, dispersion: 12, rimLight: 54, tileSize: 24,
            radius: 20, displayTarget: "primary", position: "free", surfaceMotion: 35,
            edgeSnap: true, edgeHide: false, freeHide: false, locked: false, clickThrough: false,
            captureVisible: true, animations: true, animationCurve: "easeOut", animationDurationMs: 400,
            mouseDistortion: true
        })

        readonly property var zhLabels: ["空格", "处理器", "内存", "显卡", "磁盘", "上行", "下行"]
        readonly property var enLabels: ["Blank", "CPU", "MEM", "GPU", "DISK", "UP", "DOWN"]

        function openWindow(extra) {
            sysinfo.reset()
            sysinfo.locale = extra && extra.locale ? extra.locale : "zh"
            const overrides = extra && extra.values ? extra.values : {}
            const values = Object.assign({}, baseValues, overrides)
            sysinfo.values = values
            const component = Qt.createComponent("../SettingsWindow.qml")
            compare(component.status, Component.Ready, component.errorString())
            const window = component.createObject(null, { sysinfo: sysinfo, visible: true, width: 720, height: 1000 })
            verify(window, "settings window created")
            // The layout page is taller than the default window; give it room so
            // controls are hit-testable without scrolling.
            return window
        }

        // Repeater items and view delegates both live in the `children` chain,
        // so a plain recursive walk reaches every control.
        function collect(item, out) {
            if (!item || out.indexOf(item) >= 0)
                return out
            out.push(item)
            const kids = item.children || []
            for (let i = 0; i < kids.length; ++i)
                collect(kids[i], out)
            return out
        }

        function byText(window, text) {
            const list = collect(window.contentItem, [])
            for (let i = 0; i < list.length; ++i)
                if (list[i].text === text)
                    return list[i]
            return null
        }

        function byLabel(window, labelText) {
            const list = collect(window.contentItem, [])
            for (let i = 0; i < list.length; ++i)
                if (list[i].labelText === labelText)
                    return list[i]
            return null
        }

        function sliderOf(row) {
            const kids = row.children
            for (let i = 0; i < kids.length; ++i)
                if (kids[i].visualPosition !== undefined)
                    return kids[i]
            return null
        }

        function spinOf(row) {
            const kids = row.children
            for (let i = 0; i < kids.length; ++i)
                if (kids[i].textFromValue !== undefined)
                    return kids[i]
            return null
        }

        function selectTab(window, index) {
            const tabs = findChild(window, "tabs")
            verify(tabs, "tab bar exists")
            tabs.currentIndex = index
        }

        function test_metricLabels_data() {
            return [
                { tag: "zhFollow", locale: "zh", language: "follow", expected: zhLabels },
                { tag: "enFollow", locale: "en", language: "follow", expected: enLabels },
                { tag: "zhEnglish", locale: "zh", language: "en", expected: enLabels },
                { tag: "enChinese", locale: "en", language: "zh", expected: zhLabels }
            ]
        }

        function test_metricLabels(data) {
            const window = openWindow({ locale: data.locale, values: { labelLanguage: data.language } })
            try {
                compare(JSON.stringify(window.metricLabels()), JSON.stringify(data.expected))
                const repeater = findChild(window, "metricCombos")
                verify(repeater, "cell order combos exist")
                verify(repeater.count > 0)
                compare(JSON.stringify(repeater.itemAt(0).model), JSON.stringify(data.expected))
            } finally {
                window.destroy()
            }
        }

        function test_metricOptions_data() {
            return [{ tag: "light", dark: false }, { tag: "dark", dark: true }]
        }

        function test_metricOptions(data) {
            sysinfo.look = { dark: data.dark }
            const window = openWindow({ locale: "en" })
            try {
                selectTab(window, 1)
                const metrics = findChild(window, "metricCombos")
                const combo = metrics.itemAt(0)
                verify(combo, "combo exists")
                tryCompare(combo, "visible", true)
                combo.focus = true
                mouseClick(combo)
                tryCompare(combo.popup, "opened", true)
                const list = combo.popup.contentItem
                tryVerify(() => !!list.itemAtIndex(2))
                const cpu = list.itemAtIndex(1).contentItem
                tryCompare(cpu, "text", enLabels[1])
                tryCompare(cpu, "visible", true)
                verify(cpu.width > 0 && cpu.height > 0)
                const memory = list.itemAtIndex(2)
                compare(memory.contentItem.text, enLabels[2])
                mouseClick(memory)
                tryCompare(combo, "currentText", enLabels[2])
                tryCompare(combo.popup, "opened", false)
            } finally {
                window.destroy()
            }
        }

        // The interval slider must not write back while the handle is dragged;
        // one submit lands when the mouse is released.
        function test_intervalSliderCommitsOnRelease() {
            const window = openWindow()
            try {
                selectTab(window, 0)
                const row = byLabel(window, "采样间隔（毫秒）")
                verify(row, "interval row found")
                compare(row.value, 1000)
                const slider = sliderOf(row)
                const spin = spinOf(row)
                verify(slider, "slider found")
                verify(spin, "spin box found")
                tryCompare(slider, "visible", true)

                const handleX = slider.handle.x + slider.handle.width / 2
                const handleY = slider.handle.y + slider.handle.height / 2
                mousePress(slider, handleX, handleY)
                for (let step = 1; step <= 4; ++step)
                    mouseMove(slider, handleX + step * 12, handleY)
                compare(sysinfo.edits.length, 0)
                compare(sysinfo.previews.length, 0)
                compare(row.value, 1000)
                mouseRelease(slider, handleX + 48, handleY)

                compare(sysinfo.edits.length, 1)
                compare(sysinfo.edits[0].key, "intervalMs")
                compare(sysinfo.edits[0].value, row.value)
                verify(row.value > 1000, "dragging right raises the interval")
                compare(row.value % 100, 0)
                compare(spin.value, row.value)
                compare(sysinfo.previews.length, 0)
            } finally {
                window.destroy()
            }
        }

        // The floor is 100 ms, not 500 ms.
        function test_intervalSliderReachesFloor() {
            const window = openWindow()
            try {
                selectTab(window, 0)
                const row = byLabel(window, "采样间隔（毫秒）")
                const slider = sliderOf(row)
                const handleY = slider.handle.y + slider.handle.height / 2
                mousePress(slider, slider.handle.x + slider.handle.width / 2, handleY)
                mouseMove(slider, 1, handleY)
                compare(sysinfo.edits.length, 0)
                mouseRelease(slider, 1, handleY)

                compare(sysinfo.edits.length, 1)
                compare(sysinfo.edits[0].key, "intervalMs")
                compare(sysinfo.edits[0].value, 100)
                compare(row.value, 100)
            } finally {
                window.destroy()
            }
        }

        function test_intervalSpinClampsAndSubmitsOnce() {
            const window = openWindow()
            try {
                selectTab(window, 0)
                const row = byLabel(window, "采样间隔（毫秒）")
                const spin = spinOf(row)
                spin.contentItem.text = "250"
                spin.contentItem.editingFinished()
                compare(sysinfo.edits.length, 1)
                compare(sysinfo.edits[0].value, 250)
                compare(row.value, 250)
                compare(spin.contentItem.text, "250")

                spin.contentItem.text = "20"
                spin.contentItem.editingFinished()
                compare(sysinfo.edits.length, 2)
                compare(sysinfo.edits[1].value, 100)
                compare(row.value, 100)
                compare(spin.contentItem.text, "100")
            } finally {
                window.destroy()
            }
        }

        // Size controls on the layout page preview; they do not save.
        function test_layoutRowPreviewsWithoutSaving() {
            const window = openWindow()
            try {
                selectTab(window, 1)
                const row = byLabel(window, "单元格宽度（px）")
                verify(row, "cell width row found")
                const spin = spinOf(row)
                spin.contentItem.text = "200"
                spin.contentItem.editingFinished()

                compare(sysinfo.edits.length, 0)
                compare(sysinfo.layouts.length, 0)
                compare(sysinfo.previews.length, 1)
                compare(sysinfo.previews[0].cellWidth, 200)
                verify(sysinfo.previews[0].metrics === undefined)
            } finally {
                window.destroy()
            }
        }

        function test_presetPreviewsWithoutSaving() {
            const window = openWindow()
            try {
                selectTab(window, 1)
                const cards = collect(window.contentItem, []).filter(object => object.preset !== undefined)
                compare(cards.length, 3)
                const compact = cards.filter(card => card.preset.id === "compactBar")[0]
                verify(compact, "compact bar preset card found")
                tryCompare(compact, "visible", true)
                mouseClick(compact)

                compare(sysinfo.layouts.length, 0)
                compare(sysinfo.edits.length, 0)
                compare(sysinfo.previews.length, 1)
                const preview = sysinfo.previews[0]
                compare(preview.columns, 6)
                compare(preview.cellWidth, 76)
                compare(preview.cellHeight, 28)
                compare(preview.padding, 4)
                verify(preview.tiny === true)
                verify(preview.metrics === undefined)
                verify(preview.scale === undefined)
            } finally {
                window.destroy()
            }
        }

        function test_restorePreviewsStandardGrid() {
            const window = openWindow({ values: { columns: 6, cellWidth: 76, cellHeight: 28, tiny: true, scale: 130 } })
            try {
                selectTab(window, 1)
                const restore = byText(window, "恢复默认布局")
                verify(restore, "restore default layout button found")
                tryCompare(restore, "visible", true)
                mouseClick(restore)

                compare(sysinfo.layouts.length, 0)
                compare(sysinfo.edits.length, 0)
                compare(sysinfo.previews.length, 1)
                const preview = sysinfo.previews[0]
                compare(preview.columns, 3)
                compare(preview.cellWidth, 120)
                compare(preview.cellHeight, 40)
                compare(preview.padding, 4)
                compare(preview.scale, 100)
                verify(preview.tiny === false)
            } finally {
                window.destroy()
            }
        }

        function test_applySendsWholeDraft() {
            const window = openWindow()
            try {
                selectTab(window, 1)
                const apply = byText(window, "应用布局")
                verify(apply, "apply layout button found")
                tryCompare(apply, "visible", true)
                mouseClick(apply)

                compare(sysinfo.layouts.length, 1)
                const draft = sysinfo.layouts[0]
                compare(draft.metrics, "cpu,disk,netUp;memory,gpu,netDown")
                compare(draft.columns, 3)
                compare(draft.cellWidth, 120)
                compare(draft.cellHeight, 40)
                compare(draft.padding, 4)
                compare(draft.scale, 100)
                verify(draft.tiny === false)
            } finally {
                window.destroy()
            }
        }

        function test_closeDropsDraft() {
            const window = openWindow()
            window.close()
            compare(sysinfo.cancels, 1)
            compare(window.visible, false)
            window.destroy()
        }
    }
}
