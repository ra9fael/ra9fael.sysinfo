pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 680
    height: 616
    minimumWidth: 600
    minimumHeight: 460
    visible: false
    title: root.zh ? "系统监控设置" : "System Monitor Settings"
    color: root.c("page", root.dark ? "#202020" : "#F3F3F3")

    property bool zh: sysinfo.locale !== "en"
    property bool dark: sysinfo.look.dark === true
    property bool loading: false
    property var v: ({})
    property var cells: []
    property int draftColumns: 3

    readonly property var t: zh ? zhText : enText
    readonly property var tabLabels: [t.general, t.layout, t.text, t.charts, t.material, t.interaction]
    readonly property var zhText: ({
        "general": "常用", "layout": "布局", "text": "文字", "charts": "波形",
        "material": "材质", "interaction": "交互与录屏", "close": "关闭",
        "showMonitor": "显示监视窗", "interval": "采样间隔（毫秒）", "tiny": "Tiny 紧凑模式",
        "scale": "整体缩放（%）", "lock": "锁定位置", "edgeHide": "贴边自动隐藏", "freeHide": "非贴边自动隐藏", "freeHideDelay": "非贴边收起延时（毫秒）",
        "clickThrough": "鼠标穿透", "cellOrder": "单元格顺序", "blank": "空格",
        "columns": "每行列数", "cellWidth": "单元格宽度（px）", "cellHeight": "单元格高度（px）",
        "padding": "面板内边距（px）", "dividers": "显示分隔线", "dividerOpacity": "分隔线不透明度（%）",
        "layoutPreview": "布局预览", "applyLayout": "应用布局",
        "font": "字体", "fontSize": "字号（%）", "showLabels": "显示指标名称",
        "showValues": "显示数值", "showUnits": "显示单位",
        "shape": "波形形状", "placement": "波形位置", "height": "波形高度",
        "dots": "点阵样式", "bgOpacity": "背景波形不透明度（%）",
        "belowOpacity": "下方波形不透明度（%）", "colors": "波形颜色",
        "chooseColor": "选择颜色", "shared": "统一颜色", "cpu": "CPU", "memory": "内存",
        "gpu": "GPU", "disk": "磁盘", "upload": "上传", "download": "下载",
        "theme": "配色", "glassStyle": "玻璃材质", "transparency": "通透度（%）",
        "blur": "背景模糊（%）", "refraction": "边缘折射（%）", "dispersion": "边缘色散（%）",
        "rimLight": "边缘高光（%）", "tileSize": "方格尺寸（px）", "cornerRadius": "圆角（px）",
        "display": "显示器", "position": "窗口位置", "snap": "贴边吸附",
        "animations": "交互动画", "animationCurve": "动画曲线", "animationDuration": "动画时长（毫秒）", "surfaceMotion": "表面流动（%）", "capture": "允许截图和录屏捕获",
        "auto": "自动", "off": "关闭", "inline": "折线", "wash": "面积", "dotted": "点阵",
        "bars": "细柱", "below": "文字下方", "background": "文字背景",
        "system": "跟随系统", "light": "浅色", "darkTheme": "深色",
        "liquid": "液态玻璃", "frosted": "磨砂玻璃", "square": "方格压花",
        "primary": "主显示器", "mouse": "鼠标所在显示器", "saved": "上次显示器",
        "topLeft": "左上", "topRight": "右上", "bottomLeft": "左下",
        "bottomRight": "右下", "center": "居中", "free": "自由拖动",
        "filled": "填充", "trace": "轮廓", "preset": "常用颜色", "invalidColor": "颜色需为 #RRGGBB，或六个逗号分隔的 #RRGGBB"
    })
    readonly property var enText: ({
        "general": "General", "layout": "Layout", "text": "Text", "charts": "Charts",
        "material": "Material", "interaction": "Interaction & capture", "close": "Close",
        "showMonitor": "Show monitor", "interval": "Sampling interval (ms)", "tiny": "Tiny mode",
        "scale": "Overall scale (%)", "lock": "Lock position", "edgeHide": "Auto-hide at edge", "freeHide": "Auto-hide away from edges", "freeHideDelay": "Free-position delay (ms)",
        "clickThrough": "Click-through", "cellOrder": "Cell order", "blank": "Blank",
        "columns": "Columns per row", "cellWidth": "Cell width (px)", "cellHeight": "Cell height (px)",
        "padding": "Panel padding (px)", "dividers": "Show dividers", "dividerOpacity": "Divider opacity (%)",
        "layoutPreview": "Layout preview", "applyLayout": "Apply layout",
        "font": "Font family", "fontSize": "Font size (%)", "showLabels": "Show metric labels",
        "showValues": "Show values", "showUnits": "Show units",
        "shape": "Chart shape", "placement": "Chart placement", "height": "Chart height",
        "dots": "Dot style", "bgOpacity": "Background chart opacity (%)",
        "belowOpacity": "Below-text chart opacity (%)", "colors": "Chart colours",
        "chooseColor": "Choose colour", "shared": "Shared", "cpu": "CPU", "memory": "Memory",
        "gpu": "GPU", "disk": "Disk", "upload": "Upload", "download": "Download",
        "theme": "Colour theme", "glassStyle": "Glass material", "transparency": "Transparency (%)",
        "blur": "Background blur (%)", "refraction": "Edge refraction (%)", "dispersion": "Edge dispersion (%)",
        "rimLight": "Rim light (%)", "tileSize": "Square tile size (px)", "cornerRadius": "Corner radius (px)",
        "display": "Display", "position": "Position", "snap": "Snap to edge",
        "animations": "Interaction animations", "animationCurve": "Animation curve", "animationDuration": "Animation duration (ms)", "surfaceMotion": "Surface motion (%)", "capture": "Allow screenshots and recording",
        "auto": "Automatic", "off": "Off", "inline": "Line", "wash": "Area", "dotted": "Dots",
        "bars": "Bars", "below": "Below text", "background": "Behind text",
        "system": "Follow system", "light": "Light", "darkTheme": "Dark",
        "liquid": "Liquid", "frosted": "Frosted", "square": "Square emboss",
        "primary": "Primary display", "mouse": "Display under pointer", "saved": "Last display",
        "topLeft": "Top left", "topRight": "Top right", "bottomLeft": "Bottom left",
        "bottomRight": "Bottom right", "center": "Centre", "free": "Free",
        "filled": "Filled", "trace": "Trace", "preset": "Preset colours", "invalidColor": "Use #RRGGBB, or six comma-separated #RRGGBB values"
    })

    function c(name, fallback) {
        var value = sysinfo.look[name]
        return (typeof value === "string" && value.length > 0) ? value : fallback
    }

    readonly property color textColor: c("text", dark ? "#F3F3F3" : "#1B1B1B")
    readonly property color subColor: c("subtext", dark ? "#B8B8B8" : "#666666")
    readonly property color cardColor: c("cardFill", dark ? "#2B2B2B" : "#FBFBFB")
    readonly property color edgeColor: c("cardBorder", dark ? "#1D1D1D" : "#E5E5E5")
    readonly property color separatorColor: c("divider", dark ? "#3B3B3B" : "#EAEAEA")
    readonly property color controlColor: c("control", dark ? "#373737" : "#FFFFFF")
    readonly property color controlBorderColor: c("controlBorder", dark ? "#535353" : "#D5D5D5")
    readonly property color accentColor: c("accentLine", dark ? "#60CDFF" : "#005FB8")
    readonly property color highlightedTextColor: c("highlightedText", "#FFFFFF")
    readonly property color trackColor: dark ? "#9D9D9D" : "#767676"
    readonly property int radius: 8

    function options(values) {
        var out = []
        for (var i = 0; i < values.length; i++) {
            var key = values[i] === "dark" ? "darkTheme" : values[i]
            out.push(root.t[key] || values[i])
        }
        return out
    }

    function heightOptions() {
        var out = [root.t.auto]
        var sizes = [4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 136]
        for (var i = 0; i < sizes.length; i++)
            out.push(sizes[i] + " px")
        return out
    }
    readonly property var heightValues: ["0", "4", "6", "8", "12", "16", "24", "32", "48", "64", "96", "136"]

    function metricValues() {
        return ["", "cpu", "memory", "gpu", "disk", "netUp", "netDown"]
    }

    function metricLabels() {
        return [root.t.blank, "CPU", root.t.memory, "GPU", root.t.disk, root.t.upload, root.t.download]
    }

    function indexOf(list, value) {
        for (var i = 0; i < list.length; i++)
            if (list[i] === value) return i
        return 0
    }

    function splitMetrics(text) {
        var out = String(text || "").split(/[;,\n]/)
        while (out.length < 6) out.push("")
        return out.slice(0, 6)
    }

    function metricLabel(id) {
        var values = metricValues()
        var labels = metricLabels()
        for (var i = 0; i < values.length; i++)
            if (values[i] === id) return labels[i]
        return id
    }

    function previewText() {
        var rows = []
        var columns = Math.max(1, draftColumns)
        for (var start = 0; start < cells.length; start += columns) {
            var names = []
            for (var i = start; i < Math.min(start + columns, cells.length); i++)
                names.push(metricLabel(cells[i]))
            while (names.length && names[names.length - 1] === root.t.blank) names.pop()
            if (names.length) rows.push(names.join(" · "))
        }
        return rows.join("   /   ")
    }

    function draftMetrics() {
        var rows = []
        var columns = Math.max(1, draftColumns)
        for (var start = 0; start < cells.length; start += columns) {
            var row = cells.slice(start, start + columns)
            while (row.length && row[row.length - 1] === "") row.pop()
            if (row.length) rows.push(row.join(","))
        }
        return rows.join(";")
    }

    function reload() {
        v = sysinfo.values
        loading = true
        enabledBox.checked = v.enabled === true
        intervalRow.value = v.intervalMs
        tinyBox.checked = v.tiny === true
        scaleRow.value = v.scale
        lockBox.checked = v.locked === true
        animationsBox.checked = v.animations === true
        edgeHideBox.checked = v.edgeHide === true
        freeHideBox.checked = v.freeHide === true
        freeHideDelayRow.value = v.freeHideDelayMs || 3000
        clickThroughBox.checked = v.clickThrough === true
        animationCurveRow.value = v.animationCurve || "easeOut"
        animationDurationRow.value = v.animationDurationMs || 400

        cells = splitMetrics(v.metrics)
        draftColumns = v.columns
        columnsBox.value = draftColumns
        cellWidthRow.value = v.cellWidth
        cellHeightRow.value = v.cellHeight
        paddingRow.value = v.padding
        dividersBox.checked = v.dividers === true
        dividerOpacityRow.value = v.dividerOpacity
        layoutPreview.text = previewText()

        fontBox.currentIndex = sysinfo.fontFamilies.indexOf(v.fontFamily)
        fontBox.editText = v.fontFamily
        fontSizeRow.value = v.fontSize
        labelsBox.checked = v.showLabels === true
        valuesBox.checked = v.showValues === true
        unitsBox.checked = v.showUnits === true

        shapeRow.value = v.sparklines
        placementRow.value = v.wavePlacement
        heightRow.value = String(v.waveHeight)
        dotsRow.value = v.waveDots
        waveOpacityRow.value = v.waveOpacity
        waveBelowOpacityRow.value = v.waveBelowOpacity
        colorText.text = v.waveColors
        colorText.original = v.waveColors
        colorInvalid.visible = false

        themeRow.value = v.theme
        glassRow.value = v.glassStyle
        glassRow2.value = v.glass
        blurRow.value = v.blur
        refractionRow.value = v.refraction
        dispersionRow.value = v.dispersion
        rimRow.value = v.rimLight
        tileRow.value = v.tileSize
        radiusRow.value = v.radius

        displayRow.value = v.displayTarget
        positionRow.value = v.position
        snapBox.checked = v.edgeSnap === true
        animationsBox.checked = v.animations === true
        surfaceRow.value = v.surfaceMotion
        captureBox.checked = v.captureVisible === true
        loading = false
    }

    palette.window: root.c("page", root.dark ? "#202020" : "#F3F3F3")
    palette.windowText: root.textColor
    palette.base: root.controlColor
    palette.text: root.textColor
    palette.button: root.controlColor
    palette.buttonText: root.textColor
    palette.highlight: root.accentColor
    palette.highlightedText: root.highlightedTextColor

    component ComboItem: ItemDelegate {
        id: comboItem
        required property int index
        text: typeof modelData === "string" ? modelData : modelData.text
        required property var modelData
        width: ListView.view.width
        highlighted: ListView.isCurrentItem
        contentItem: Label {
            text: comboItem.text
            color: comboItem.highlighted ? root.highlightedTextColor : root.textColor
            font.pixelSize: 14
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            color: comboItem.highlighted ? root.accentColor : root.controlColor
        }
    }

    component NumberRow: RowLayout {
        id: numberRow
        property string labelText: ""
        property int from: 0
        property int to: 100
        property int stepSize: 1
        property int value: 0
        signal edited(int value)
        signal previewed(int value)
        signal committed()
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.preferredHeight: 42
        spacing: 12
        Label {
            text: numberRow.labelText
            color: root.textColor
            font.pixelSize: 14
            Layout.preferredWidth: 284
            Layout.minimumWidth: 150
            elide: Text.ElideRight
        }
        Slider {
            id: slider
            Layout.fillWidth: true
            Layout.minimumWidth: 52
            Layout.preferredHeight: 32
            from: numberRow.from
            to: numberRow.to
            stepSize: numberRow.stepSize
            value: numberRow.value
            onMoved: {
                if (root.loading) return
                if (pressed) numberRow.previewed(Math.round(value))
                else numberRow.edited(Math.round(value))
            }
            onPressedChanged: if (!pressed && !root.loading) numberRow.committed()
            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: slider.availableWidth
                height: 4
                radius: 2
                color: root.c("track", root.dark ? "#454545" : "#E5E5E5")
                Rectangle {
                    width: slider.visualPosition * parent.width
                    height: parent.height
                    radius: parent.radius
                    color: root.accentColor
                }
            }
            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: 20
                height: 20
                radius: 10
                color: root.controlColor
                border.color: root.trackColor
                border.width: 1
                Rectangle {
                    anchors.centerIn: parent
                    width: 10
                    height: 10
                    radius: 5
                    color: root.accentColor
                }
            }
        }
        SpinBox {
            id: spin
            Layout.preferredWidth: 132
            Layout.minimumWidth: 132
            Layout.preferredHeight: 32
            from: numberRow.from
            to: numberRow.to
            stepSize: numberRow.stepSize
            value: numberRow.value
            editable: true
            font.pixelSize: 14
            palette.text: root.textColor
            palette.base: root.controlColor
            palette.button: root.controlColor
            palette.buttonText: root.subColor
            contentItem: TextInput {
                text: spin.displayText
                font: spin.font
                color: root.textColor
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                selectByMouse: true
                readOnly: !spin.editable
                validator: spin.validator
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onEditingFinished: spin.value = spin.valueFromText(text, spin.locale)
            }
            onValueModified: if (!root.loading) numberRow.edited(value)
            background: Rectangle {
                radius: 4
                color: root.controlColor
                border.color: spin.activeFocus ? root.accentColor : root.controlBorderColor
                border.width: spin.activeFocus ? 2 : 1
            }
        }
        onValueChanged: {
            slider.value = value
            spin.value = value
        }
    }

    component ChoiceRow: RowLayout {
        id: choiceRow
        property string labelText: ""
        property var values: []
        property var labels: []
        property string value: ""
        signal chosen(string value)
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.preferredHeight: 44
        spacing: 12
        Label {
            text: choiceRow.labelText
            color: root.textColor
            font.pixelSize: 14
            Layout.fillWidth: true
            Layout.minimumWidth: 120
            elide: Text.ElideRight
        }
        ComboBox {
            id: combo
            Layout.preferredWidth: 182
            Layout.minimumWidth: 142
            Layout.preferredHeight: 32
            model: choiceRow.labels
            delegate: ComboItem {}
            currentIndex: root.indexOf(choiceRow.values, choiceRow.value)
            font.pixelSize: 14
            palette.text: root.textColor
            palette.buttonText: root.textColor
            palette.button: root.controlColor
            onActivated: if (!root.loading) choiceRow.chosen(choiceRow.values[currentIndex])
            onModelChanged: currentIndex = root.indexOf(choiceRow.values, choiceRow.value)
            contentItem: Label {
                leftPadding: 12
                rightPadding: 32
                text: combo.displayText
                color: root.textColor
                font: combo.font
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            indicator: Canvas {
                x: combo.width - width - 12
                y: (combo.height - height) / 2
                width: 12
                height: 8
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = root.subColor
                    ctx.lineWidth = 1.5
                    ctx.beginPath()
                    ctx.moveTo(2, 2)
                    ctx.lineTo(6, 6)
                    ctx.lineTo(10, 2)
                    ctx.stroke()
                }
            }
            background: Rectangle {
                radius: 4
                color: combo.hovered ? root.c("controlHover", root.dark ? "#3C3C3C" : "#F6F6F6") : root.controlColor
                border.color: combo.activeFocus ? root.accentColor : root.controlBorderColor
                border.width: combo.activeFocus ? 2 : 1
            }
        }
        onValueChanged: combo.currentIndex = root.indexOf(values, value)
    }

    component CheckRow: RowLayout {
        id: checkRow
        property string labelText: ""
        property alias checked: toggle.checked
        signal changed(bool checked)
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.preferredHeight: 48
        spacing: 8
        Label {
            text: checkRow.labelText
            color: root.textColor
            font.pixelSize: 14
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Label {
            text: toggle.checked ? (root.zh ? "开" : "On") : (root.zh ? "关" : "Off")
            color: root.subColor
            font.pixelSize: 13
        }
        Switch {
            id: toggle
            Accessible.name: checkRow.labelText
            implicitWidth: 40
            implicitHeight: 24
            onToggled: checkRow.changed(checked)
            indicator: Rectangle {
                x: (toggle.width - width) / 2
                y: (toggle.height - height) / 2
                width: 40
                height: 20
                radius: 10
                color: toggle.checked ? root.accentColor : "transparent"
                border.color: toggle.checked ? root.accentColor : root.trackColor
                Rectangle {
                    x: toggle.checked ? parent.width - width - 4 : 4
                    y: 4
                    width: 12
                    height: 12
                    radius: 6
                    color: toggle.checked ? "#FFFFFF" : root.subColor
                }
            }
            background: Item {}
        }
    }

    component SettingDivider: Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: 1
        Layout.rightMargin: 1
        Layout.preferredHeight: 1
        color: root.separatorColor
    }

    component SettingsCard: ColumnLayout {
        id: settingsCard
        property string titleText: ""
        default property alias content: cardContent.data
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: 8

        Label {
            text: settingsCard.titleText
            color: root.textColor
            font.pixelSize: 14
            font.weight: Font.DemiBold
            Layout.leftMargin: 1
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: cardContent.implicitHeight + 2
            radius: root.radius
            color: root.cardColor
            border.color: root.edgeColor
            border.width: 1
            ColumnLayout {
                id: cardContent
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0
            }
        }
    }

    onClosing: {
        close.accepted = false
        sysinfo.commitPreview()
        root.visible = false
    }

    Connections {
        target: sysinfo
        function onValuesChanged() { root.reload() }
        function onAppearanceChanged() { root.reload() }
    }

    Component.onCompleted: reload()

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 4

        Label {
            text: root.zh ? "系统监控" : "System Monitor"
            color: root.textColor
            font.pixelSize: 28
            font.weight: Font.DemiBold
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            verticalAlignment: Text.AlignVCenter
        }

        TabBar {
            id: tabs
            objectName: "tabs"
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            spacing: 0
            background: Rectangle {
                color: "transparent"
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: root.separatorColor
                }
            }
            Repeater {
                model: root.tabLabels
                TabButton {
                    id: tabButton
                    required property int index
                    required property string modelData
                    text: modelData
                    implicitWidth: Math.ceil(metrics.advanceWidth) + 24
                    TextMetrics {
                        id: metrics
                        text: tabButton.text
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    contentItem: Label {
                        text: tabButton.text
                        color: tabButton.checked ? root.textColor : root.subColor
                        font.pixelSize: 14
                        font.weight: tabButton.checked ? Font.DemiBold : Font.Normal
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Item {
                        Rectangle {
                            visible: tabs.currentIndex === tabButton.index
                            width: parent.width
                            height: 3
                            radius: 1.5
                            color: root.accentColor
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                        }
                    }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex
            clip: true

            ScrollView {
                contentWidth: availableWidth
                clip: true
                padding: 0
                background: Rectangle {
                    color: "transparent"
                }
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ColumnLayout {
                    width: parent.width
                    Layout.minimumWidth: 0
                    spacing: 12

                    SettingsCard { titleText: root.zh ? "监视窗" : "Monitor"
                        CheckRow { id: enabledBox; labelText: root.t.showMonitor
                            onChanged: if (!root.loading) sysinfo.edit("enabled", checked) }
                        SettingDivider {}
                        NumberRow { id: intervalRow; labelText: root.t.interval; from: 500; to: 5000; stepSize: 100
                            onEdited: sysinfo.edit("intervalMs", value); onPreviewed: sysinfo.preview("intervalMs", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        CheckRow { id: tinyBox; labelText: root.t.tiny
                            onChanged: if (!root.loading) sysinfo.edit("tiny", checked) }
                        SettingDivider {}
                        NumberRow { id: scaleRow; labelText: root.t.scale; from: 80; to: 200
                            onEdited: sysinfo.edit("scale", value); onPreviewed: sysinfo.preview("scale", value); onCommitted: sysinfo.commitPreview() }
                    }
                    SettingsCard { titleText: root.t.interaction
                        CheckRow { id: lockBox; labelText: root.t.lock
                            onChanged: if (!root.loading) sysinfo.edit("locked", checked) }
                        SettingDivider {}
                        CheckRow { id: edgeHideBox; labelText: root.t.edgeHide
                            onChanged: if (!root.loading) sysinfo.edit("edgeHide", checked) }
                        SettingDivider {}
                        CheckRow { id: freeHideBox; labelText: root.t.freeHide
                            onChanged: if (!root.loading) sysinfo.edit("freeHide", checked) }
                        SettingDivider {}
                        NumberRow { id: freeHideDelayRow; labelText: root.t.freeHideDelay; from: 500; to: 30000; stepSize: 100
                            onEdited: sysinfo.edit("freeHideDelayMs", value); onPreviewed: sysinfo.preview("freeHideDelayMs", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        CheckRow { id: clickThroughBox; labelText: root.t.clickThrough
                            onChanged: if (!root.loading) sysinfo.edit("clickThrough", checked) }
                    }
                }
            }

            ScrollView {
                contentWidth: availableWidth
                clip: true
                padding: 0
                background: Rectangle {
                    color: "transparent"
                }
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ColumnLayout {
                    width: parent.width
                    Layout.minimumWidth: 0
                    spacing: 12

                    SettingsCard { titleText: root.t.cellOrder
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.topMargin: 12
                            Layout.bottomMargin: 8
                            columns: 3
                            columnSpacing: 8
                            rowSpacing: 4
                            Repeater {
                                objectName: "metricCombos"
                                model: 6
                                ComboBox {
                                    id: cellCombo
                                    required property int index
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    model: root.metricLabels()
                                    delegate: ComboItem {}
                                    currentIndex: 0
                                    font.pixelSize: 14
                                    palette.text: root.textColor
                                    palette.button: root.controlColor
                                    property int wantedIndex: root.indexOf(root.metricValues(), root.cells[index] || "")
                                    Binding { target: cellCombo; property: "currentIndex"; value: cellCombo.wantedIndex }
                                    onActivated: function(itemIndex) {
                                        if (root.loading) return
                                        root.cells[cellCombo.index] = root.metricValues()[itemIndex]
                                        layoutPreview.text = root.previewText()
                                    }
                                    onModelChanged: currentIndex = root.indexOf(root.metricValues(), root.cells[cellCombo.index] || "")
                                    background: Rectangle {
                                        radius: 4
                                        color: cellCombo.hovered ? root.c("controlHover", root.dark ? "#3C3C3C" : "#F6F6F6") : root.controlColor
                                        border.color: cellCombo.activeFocus ? root.accentColor : root.controlBorderColor
                                    }
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.preferredHeight: 48
                            Label { text: root.t.columns; color: root.textColor; font.pixelSize: 14; Layout.fillWidth: true }
                            SpinBox { id: columnsBox; from: 1; to: 6; editable: true; Layout.preferredWidth: 104; Layout.minimumWidth: 104; Layout.preferredHeight: 32
                                font.pixelSize: 14
                                palette.text: root.textColor
                                palette.base: root.controlColor
                                background: Rectangle { radius: 4; color: root.controlColor; border.color: columnsBox.activeFocus ? root.accentColor : root.controlBorderColor; border.width: columnsBox.activeFocus ? 2 : 1 }
                                onValueModified: { root.draftColumns = value; layoutPreview.text = root.previewText() } }
                        }
                    }
                    SettingsCard { titleText: root.t.layout
                        NumberRow { id: cellWidthRow; labelText: root.t.cellWidth; from: 80; to: 320
                            onEdited: sysinfo.edit("cellWidth", value); onPreviewed: sysinfo.preview("cellWidth", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: cellHeightRow; labelText: root.t.cellHeight; from: 28; to: 160
                            onEdited: sysinfo.edit("cellHeight", value); onPreviewed: sysinfo.preview("cellHeight", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: paddingRow; labelText: root.t.padding; from: 0; to: 40
                            onEdited: sysinfo.edit("padding", value); onPreviewed: sysinfo.preview("padding", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        CheckRow { id: dividersBox; labelText: root.t.dividers
                            onChanged: if (!root.loading) sysinfo.edit("dividers", checked) }
                        SettingDivider {}
                        NumberRow { id: dividerOpacityRow; labelText: root.t.dividerOpacity; from: 0; to: 100
                            onEdited: sysinfo.edit("dividerOpacity", value); onPreviewed: sysinfo.preview("dividerOpacity", value); onCommitted: sysinfo.commitPreview() }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.preferredHeight: 56
                        spacing: 10
                        Label {
                            id: layoutPreview
                            color: root.textColor
                            text: root.previewText()
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Button {
                            text: root.t.applyLayout
                            Layout.preferredWidth: 108
                            Layout.preferredHeight: 32
                            palette.button: root.accentColor
                            palette.buttonText: "#FFFFFF"
                            onClicked: sysinfo.applyLayout(root.draftMetrics(), root.draftColumns)
                        }
                    }
                }
            }

            ScrollView {
                contentWidth: availableWidth
                clip: true
                padding: 0
                background: Rectangle {
                    color: "transparent"
                }
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ColumnLayout {
                    width: parent.width
                    Layout.minimumWidth: 0
                    spacing: 12

                    SettingsCard { titleText: root.t.text
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.preferredHeight: 44
                            Label { text: root.t.font; color: root.textColor; font.pixelSize: 14; Layout.fillWidth: true }
                            ComboBox { id: fontBox; Layout.preferredWidth: 182; Layout.minimumWidth: 142; Layout.preferredHeight: 32; editable: true; model: sysinfo.fontFamilies
                                font.pixelSize: 14
                                palette.text: root.textColor
                                palette.button: root.controlColor
                                delegate: ComboItem {}
                                contentItem: TextField {
                                    leftPadding: 12
                                    rightPadding: fontBox.indicator.width + 12
                                    text: fontBox.editText
                                    font: fontBox.font
                                    color: root.textColor
                                    selectionColor: root.accentColor
                                    selectedTextColor: root.dark ? "#FFFFFF" : "#FFFFFF"
                                    verticalAlignment: TextInput.AlignVCenter
                                    background: Item {}
                                    onTextEdited: fontBox.editText = text
                                }
                                indicator: Canvas {
                                    x: fontBox.width - width - 12
                                    y: (fontBox.height - height) / 2
                                    width: 12
                                    height: 8
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        ctx.strokeStyle = root.subColor
                                        ctx.lineWidth = 1.5
                                        ctx.beginPath()
                                        ctx.moveTo(2, 2)
                                        ctx.lineTo(6, 6)
                                        ctx.lineTo(10, 2)
                                        ctx.stroke()
                                    }
                                }
                                background: Rectangle { radius: 4; color: fontBox.hovered ? root.c("controlHover", root.dark ? "#3C3C3C" : "#F6F6F6") : root.controlColor; border.color: fontBox.activeFocus ? root.accentColor : root.controlBorderColor; border.width: fontBox.activeFocus ? 2 : 1 }
                                onActivated: if (!root.loading) sysinfo.edit("fontFamily", currentText)
                                onAccepted: if (!root.loading && editText.trim().length) sysinfo.edit("fontFamily", editText.trim()) }
                        }
                        SettingDivider {}
                        NumberRow { id: fontSizeRow; labelText: root.t.fontSize; from: 70; to: 180
                            onEdited: sysinfo.edit("fontSize", value); onPreviewed: sysinfo.preview("fontSize", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        CheckRow { id: labelsBox; labelText: root.t.showLabels
                            onChanged: if (!root.loading) sysinfo.edit("showLabels", checked) }
                        SettingDivider {}
                        CheckRow { id: valuesBox; labelText: root.t.showValues
                            onChanged: if (!root.loading) sysinfo.edit("showValues", checked) }
                        SettingDivider {}
                        CheckRow { id: unitsBox; labelText: root.t.showUnits
                            onChanged: if (!root.loading) sysinfo.edit("showUnits", checked) }
                    }
                }
            }

            ScrollView {
                contentWidth: availableWidth
                clip: true
                padding: 0
                background: Rectangle {
                    color: "transparent"
                }
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ColumnLayout {
                    width: parent.width
                    Layout.minimumWidth: 0
                    spacing: 12

                    SettingsCard { titleText: root.zh ? "形状与位置" : "Shape & placement"
                        ChoiceRow { id: shapeRow; labelText: root.t.shape; values: ["off", "inline", "wash", "dotted", "bars"]
                            labels: root.options(["off", "inline", "wash", "dotted", "bars"]); onChosen: sysinfo.edit("sparklines", value) }
                        SettingDivider {}
                        ChoiceRow { id: placementRow; labelText: root.t.placement; values: ["auto", "below", "background"]
                            labels: root.options(["auto", "below", "background"]); onChosen: sysinfo.edit("wavePlacement", value) }
                        SettingDivider {}
                        ChoiceRow { id: heightRow; labelText: root.t.height; values: root.heightValues; labels: root.heightOptions()
                            onChosen: sysinfo.edit("waveHeight", Number(value)) }
                        SettingDivider {}
                        ChoiceRow { id: dotsRow; labelText: root.t.dots; values: ["filled", "trace"]
                            labels: root.options(["filled", "trace"]); onChosen: sysinfo.edit("waveDots", value) }
                    }
                    SettingsCard { titleText: root.zh ? "不透明度与颜色" : "Opacity & colour"
                        NumberRow { id: waveOpacityRow; labelText: root.t.bgOpacity; from: 0; to: 100
                            onEdited: sysinfo.edit("waveOpacity", value); onPreviewed: sysinfo.preview("waveOpacity", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: waveBelowOpacityRow; labelText: root.t.belowOpacity; from: 0; to: 100
                            onEdited: sysinfo.edit("waveBelowOpacity", value); onPreviewed: sysinfo.preview("waveBelowOpacity", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.preferredHeight: 60
                            spacing: 8
                            Label { text: root.t.colors; color: root.textColor; font.pixelSize: 14; Layout.fillWidth: true }
                            ComboBox { id: colorTarget; Layout.preferredWidth: 122; Layout.preferredHeight: 32
                                font.pixelSize: 14
                                model: [root.t.shared, root.t.cpu, root.t.memory, root.t.gpu, root.t.disk, root.t.upload, root.t.download]
                                delegate: ComboItem {}
                                palette.text: root.textColor
                                palette.button: root.controlColor
                                background: Rectangle { radius: 4; color: root.controlColor; border.color: root.controlBorderColor }
                            }
                            Row { spacing: 4
                                Repeater {
                                    model: ["#36BFFA", "#A78BFA", "#34D399", "#FBBF24", "#FB7185", "#60A5FA"]
                                    Rectangle { required property string modelData; width: 12; height: 12; radius: 2; color: modelData; border.color: root.controlBorderColor; anchors.verticalCenter: parent.verticalCenter }
                                }
                            }
                            Button { text: root.t.chooseColor; Layout.preferredWidth: 84; Layout.preferredHeight: 32; onClicked: colorPopup.open() }
                            TextField { id: colorText; visible: false; property string original: ""
                                onEditingFinished: {
                                    if (root.loading) return
                                    if (/^#[0-9A-Fa-f]{6}$/.test(text) || /^#[0-9A-Fa-f]{6}(,#[0-9A-Fa-f]{6}){5}$/.test(text)) {
                                        colorInvalid.visible = false
                                        sysinfo.edit("waveColors", text)
                                    } else {
                                        colorInvalid.visible = true
                                        text = original
                                    }
                                }
                            }
                        }
                        Label { id: colorInvalid; visible: false; text: root.t.invalidColor; color: "#C42B1C"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    }
                }
            }

            ScrollView {
                contentWidth: availableWidth
                clip: true
                padding: 0
                background: Rectangle {
                    color: "transparent"
                }
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ColumnLayout {
                    width: parent.width
                    Layout.minimumWidth: 0
                    spacing: 12

                    SettingsCard { titleText: root.t.material
                        ChoiceRow { id: themeRow; labelText: root.t.theme; values: ["system", "light", "dark"]; labels: root.options(["system", "light", "dark"]); onChosen: sysinfo.edit("theme", value) }
                        SettingDivider {}
                        ChoiceRow { id: glassRow; labelText: root.t.glassStyle; values: ["liquid", "frosted", "square"]; labels: root.options(["liquid", "frosted", "square"]); onChosen: sysinfo.edit("glassStyle", value) }
                        SettingDivider {}
                        NumberRow { id: glassRow2; labelText: root.t.transparency; from: 0; to: 100; onEdited: sysinfo.edit("glass", value); onPreviewed: sysinfo.preview("glass", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: blurRow; labelText: root.t.blur; from: 0; to: 100; onEdited: sysinfo.edit("blur", value); onPreviewed: sysinfo.preview("blur", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: refractionRow; labelText: root.t.refraction; from: 0; to: 100; onEdited: sysinfo.edit("refraction", value); onPreviewed: sysinfo.preview("refraction", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: dispersionRow; labelText: root.t.dispersion; from: 0; to: 100; onEdited: sysinfo.edit("dispersion", value); onPreviewed: sysinfo.preview("dispersion", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: rimRow; labelText: root.t.rimLight; from: 0; to: 100; onEdited: sysinfo.edit("rimLight", value); onPreviewed: sysinfo.preview("rimLight", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: tileRow; labelText: root.t.tileSize; from: 8; to: 80; onEdited: sysinfo.edit("tileSize", value); onPreviewed: sysinfo.preview("tileSize", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: radiusRow; labelText: root.t.cornerRadius; from: 8; to: 28; onEdited: sysinfo.edit("radius", value); onPreviewed: sysinfo.preview("radius", value); onCommitted: sysinfo.commitPreview() }
                    }
                }
            }

            ScrollView {
                contentWidth: availableWidth
                clip: true
                padding: 0
                background: Rectangle {
                    color: "transparent"
                }
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ColumnLayout {
                    width: parent.width
                    Layout.minimumWidth: 0
                    spacing: 12

                    SettingsCard { titleText: root.t.interaction
                        ChoiceRow { id: displayRow; labelText: root.t.display; values: ["primary", "mouse", "saved"]; labels: root.options(["primary", "mouse", "saved"]); onChosen: sysinfo.edit("displayTarget", value) }
                        SettingDivider {}
                        ChoiceRow { id: positionRow; labelText: root.t.position; values: ["topLeft", "topRight", "bottomLeft", "bottomRight", "center", "free"]; labels: root.options(["topLeft", "topRight", "bottomLeft", "bottomRight", "center", "free"]); onChosen: sysinfo.edit("position", value) }
                        SettingDivider {}
                        CheckRow { id: snapBox; labelText: root.t.snap; onChanged: if (!root.loading) sysinfo.edit("edgeSnap", checked) }
                        SettingDivider {}
                        CheckRow { id: animationsBox; labelText: root.t.animations; onChanged: if (!root.loading) sysinfo.edit("animations", checked) }
                        SettingDivider {}
                        ChoiceRow { id: animationCurveRow; labelText: root.t.animationCurve; values: ["linear", "easeOut", "easeInOut", "easeIn"]; labels: root.zh ? ["线性", "缓出", "缓入缓出", "缓入"] : ["Linear", "Ease out", "Ease in-out", "Ease in"]; onChosen: sysinfo.edit("animationCurve", value) }
                        SettingDivider {}
                        NumberRow { id: animationDurationRow; labelText: root.t.animationDuration; from: 100; to: 1500; stepSize: 50; onEdited: sysinfo.edit("animationDurationMs", value); onPreviewed: sysinfo.preview("animationDurationMs", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        NumberRow { id: surfaceRow; labelText: root.t.surfaceMotion; from: 0; to: 100; onEdited: sysinfo.edit("surfaceMotion", value); onPreviewed: sysinfo.preview("surfaceMotion", value); onCommitted: sysinfo.commitPreview() }
                        SettingDivider {}
                        CheckRow { id: captureBox; labelText: root.t.capture; onChanged: if (!root.loading) sysinfo.edit("captureVisible", checked) }
                    }
                }
            }
        }

    }

    Popup {
        id: colorPopup
        parent: Overlay.overlay
        modal: true
        focus: true
        width: 340
        height: 260
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)

        background: Rectangle {
            color: root.cardColor
            radius: root.radius
            border.color: root.edgeColor
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10
            Label {
                text: root.t.preset
                color: root.textColor
            }
            GridLayout {
                columns: 8
                columnSpacing: 6
                rowSpacing: 6
                Repeater {
                    model: ["#36BFFA", "#A78BFA", "#34D399", "#FBBF24", "#FB7185", "#60A5FA",
                            "#F97316", "#EF4444", "#EC4899", "#8B5CF6", "#22C55E", "#14B8A6",
                            "#0EA5E9", "#6366F1", "#F59E0B", "#84CC16", "#E11D48", "#64748B",
                            "#94A3B8", "#CBD5E1", "#F8FAFC", "#1E293B", "#000000", "#FFFFFF"]
                    Rectangle {
                        required property var modelData
                        width: 30
                        height: 30
                        radius: 6
                        color: modelData
                        border.color: root.edgeColor
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                root.applyColour(modelData)
                                colorPopup.close()
                            }
                        }
                    }
                }
            }
        }
    }

    function applyColour(hex) {
        var parts = String(colorText.text || "").split(",")
        var target = colorTarget.currentIndex
        var values
        if (target === 0) {
            values = [hex]
        } else {
            if (parts.length === 6) {
                values = parts.slice()
            } else {
                var base = (parts.length === 1 && parts[0].trim().length) ? parts[0].trim() : hex
                values = [base, base, base, base, base, base]
            }
            values[target - 1] = hex
        }
        var text = values.join(",")
        colorText.text = text
        colorText.original = text
        sysinfo.edit("waveColors", text)
    }
}

