import QtQuick
import SysInfo 1.0

// 悬浮资源监视窗：一块液态玻璃 + 2×3 指标网格 + 可选微型波形。
Window {
    id: root
    objectName: "sysinfoOverlay"
    color: "transparent"
    visible: false
    width: bridge.windowWidth
    height: bridge.windowHeight
    title: qsTr("系统信息")
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus

    readonly property real s: bridge.scale / 100
    readonly property bool dark: bridge.dark
    readonly property color ink: dark ? "#ffffff" : "#101419"
    readonly property color muted: dark ? Qt.rgba(1, 1, 1, 0.66) : Qt.rgba(0.06, 0.08, 0.10, 0.58)
    readonly property color line: dark ? Qt.rgba(1, 1, 1, 0.20) : Qt.rgba(0.06, 0.08, 0.10, 0.13)
    readonly property string fontFamily: bridge.fontFamily
    readonly property int gridColumns: bridge.gridColumns
    readonly property int gridRows: bridge.gridRows
    readonly property real edgeReveal: 26 * s
    readonly property real edgeDiameter: edgeReveal * 2
    readonly property real contentShiftX: !bridge.edgeCollapsed ? 0
        : bridge.edgeSide === "left" ? -width
        : bridge.edgeSide === "right" ? width : 0
    readonly property real contentShiftY: !bridge.edgeCollapsed ? 0
        : bridge.edgeSide === "top" ? -height
        : bridge.edgeSide === "bottom" ? height : 0
    function touchesEdge() {
        const area = bridge.availableArea()
        const gap = 8
        return x <= area.left + gap || y <= area.top + gap
                || x + width >= area.right - gap || y + height >= area.bottom - gap
    }
    Behavior on opacity { NumberAnimation { duration: bridge.animations ? 160 : 0; easing.type: Easing.OutCubic } }

    GlassImage {
        x: root.contentShiftX
        y: root.contentShiftY
        width: root.width
        height: root.height
        image: overlayGlass.image
        Behavior on x { NumberAnimation { duration: bridge.animations ? 180 : 0; easing.type: Easing.OutCubic } }
        Behavior on y { NumberAnimation { duration: bridge.animations ? 180 : 0; easing.type: Easing.OutCubic } }
    }

    Item {
        id: panel
        x: Math.round(bridge.margin + root.contentShiftX)
        y: Math.round(bridge.margin + root.contentShiftY)
        width: Math.round(bridge.panelWidth)
        height: Math.round(bridge.panelHeight)
        Behavior on x { NumberAnimation { duration: bridge.animations ? 180 : 0; easing.type: Easing.OutCubic } }
        Behavior on y { NumberAnimation { duration: bridge.animations ? 180 : 0; easing.type: Easing.OutCubic } }

        readonly property real cellW: width / root.gridColumns
        readonly property real cellH: height / root.gridRows

        Repeater {
            model: bridge.slotList

            delegate: Item {
                id: cell
                required property int index
                required property var modelData

                x: Math.round(modelData.column * panel.cellW)
                y: Math.round(modelData.row * panel.cellH)
                width: Math.round(panel.cellW)
                height: Math.round(panel.cellH)

                // 波形画在文字下面
                Canvas {
                    id: spark
                    readonly property bool wash: bridge.sparklines === "wash"
                    readonly property bool firstColumn: index % root.gridColumns === 0
                    readonly property bool lastColumn: index % root.gridColumns === root.gridColumns - 1 || index === bridge.slotList.length - 1
                    readonly property bool firstRow: index < root.gridColumns
                    readonly property bool lastRow: index >= bridge.slotList.length - root.gridColumns
                    readonly property real safeInset: Math.min(bridge.radius * root.s, Math.min(cell.width, cell.height) / 2)
                    property real rateRange: 1
                    x: wash ? (firstColumn ? safeInset : 0) : 10 * root.s
                    y: wash ? (firstRow ? safeInset : 0) : cell.height - 19 * root.s
                    width: wash ? cell.width - (firstColumn ? safeInset : 0) - (lastColumn ? safeInset : 0) : cell.width - 20 * root.s
                    height: wash ? cell.height - (firstRow ? safeInset : 0) - (lastRow ? safeInset : 0) : 15 * root.s
                    visible: !modelData.empty && bridge.sparklines !== "off" && modelData.history.length > 1
                    opacity: wash ? 0.55 : 0.9

                    Connections {
                        target: bridge
                        function onSnapshotChanged() { spark.requestPaint() }
                    }
                    Component.onCompleted: requestPaint()
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()

                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        const hist = modelData.history
                        if (!hist || hist.length < 2)
                            return
                        let lo = 0
                        let hi = modelData.rate ? 1 : 100
                        if (modelData.rate) {
                            const peak = Math.max.apply(null, hist)
                            rateRange = Math.max(1, peak * 1.15, rateRange * 0.985)
                            hi = rateRange
                        }
                        const w = width, h = height
                        const stroke = wash ? (root.dark ? "rgba(255,255,255,0.75)" : "rgba(16,20,25,0.55)")
                                            : (root.dark ? "rgba(255,255,255,0.92)" : "rgba(16,20,25,0.80)")
                        ctx.beginPath()
                        for (let i = 0; i < hist.length; ++i) {
                            const px = (i / (hist.length - 1)) * w
                            const py = 1 + (h - 2) * (1 - (hist[i] - lo) / (hi - lo))
                            if (i === 0)
                                ctx.moveTo(px, py)
                            else
                                ctx.lineTo(px, py)
                        }
                        ctx.lineWidth = wash ? 1.0 : 1.2
                        ctx.strokeStyle = stroke
                        ctx.lineJoin = "round"
                        ctx.lineCap = "round"
                        ctx.stroke()
                        if (wash) {
                            ctx.lineTo(w, h)
                            ctx.lineTo(0, h)
                            ctx.closePath()
                            ctx.fillStyle = root.dark ? "rgba(255,255,255,0.10)" : "rgba(16,20,25,0.08)"
                            ctx.fill()
                        }
                    }
                }

                // 分隔线
                Rectangle {
                    visible: !modelData.empty && modelData.column !== 0
                    x: 0
                    y: 6 * root.s
                    width: 1
                    height: cell.height - 12 * root.s
                    color: root.line
                }
                Rectangle {
                    visible: !modelData.empty && modelData.row < root.gridRows - 1
                    x: 6 * root.s
                    y: cell.height - 1
                    width: cell.width - 12 * root.s
                    height: 1
                    color: root.line
                }

                Text {
                    id: label
                    anchors.left: parent.left
                    anchors.leftMargin: 11 * root.s
                    anchors.top: parent.top
                    anchors.topMargin: (bridge.sparklines === "inline" ? 6 : 7) * root.s
                    visible: !cell.modelData.empty
                    text: cell.modelData.label
                    color: root.muted
                    font.family: root.fontFamily
                    font.pixelSize: Math.round(11.5 * root.s * bridge.fontSize / 100 * 10) / 10
                    font.weight: Font.Medium
                }

                Item {
                    id: valueBox
                    anchors.left: parent.left
                    anchors.leftMargin: 11 * root.s
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: (bridge.sparklines === "inline" ? 18 : 8) * root.s
                    width: parent.width - 22 * root.s
                    height: value.implicitHeight

                    visible: !cell.modelData.empty
                    Text {
                        id: value
                        anchors.left: parent.left
                        anchors.right: unit.visible ? unit.left : parent.right
                        anchors.rightMargin: unit.visible ? 3 * root.s : 0
                        anchors.top: parent.top
                        text: cell.modelData.value
                        elide: Text.ElideRight
                        color: root.ink
                        opacity: cell.modelData.fresh ? 1 : 0.72
                        font.family: root.fontFamily
                        font.pixelSize: Math.round(13.5 * root.s * bridge.fontSize / 100 * 10) / 10
                        font.weight: Font.DemiBold
                    }
                    Text {
                        id: unit
                        anchors.right: parent.right
                        anchors.baseline: value.baseline
                        text: cell.modelData.unit
                        visible: text.length > 0
                        color: root.muted
                        font.family: root.fontFamily
                        font.pixelSize: Math.round(9.5 * root.s * bridge.fontSize / 100 * 10) / 10
                    }
                }

            }
        }

        // 数据陈旧时右下角一个提醒点
        Rectangle {
            visible: bridge.statusText.length > 0
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 5 * root.s
            width: 5 * root.s
            height: 5 * root.s
            radius: width / 2
            color: "#f2b25c"
        }
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        enabled: !bridge.clickThrough
        hoverEnabled: true

        property real grabOffsetX: 0
        property real grabOffsetY: 0
        property bool pressedHere: false

        onPressed: function (mouse) {
            if (mouse.button === Qt.RightButton) {
                const p = bridge.cursorPos()
                menu.popupAt(p.x, p.y)
                return
            }
            if (mouse.button === Qt.MiddleButton) {
                bridge.edit("enabled", false)
                return
            }
            if (bridge.locked)
                return
            pressedHere = true
            const p = bridge.cursorPos()
            grabOffsetX = root.x - p.x
            grabOffsetY = root.y - p.y
        }
        onPositionChanged: function (mouse) {
            if (!pressedHere || bridge.locked)
                return
            const p = bridge.cursorPos()
            root.x = Math.round(p.x + grabOffsetX)
            root.y = Math.round(p.y + grabOffsetY)
        }
        onReleased: {
            if (pressedHere) {
                pressedHere = false
                bridge.noteMoved()
            }
        }
        onEntered: bridge.setEdgeCollapsed(false)
        onExited: if (bridge.edgeHide && root.touchesEdge() && !pressedHere) edgeTimer.restart()
        onDoubleClicked: bridge.setPaused(!bridge.paused)
        onWheel: function (wheel) {
            if ((wheel.modifiers & Qt.ControlModifier) === 0)
                return
            const step = wheel.angleDelta.y > 0 ? 5 : -5
            bridge.edit("scale", Math.max(80, Math.min(200, bridge.scale + step)))
        }
    }

    Timer {
        id: edgeTimer
        interval: 700
        repeat: false
        onTriggered: if (bridge.edgeHide && root.touchesEdge()) bridge.setEdgeCollapsed(true)
    }

    Rectangle {
        id: edgeStrip
        visible: bridge.edgeCollapsed
        width: root.edgeDiameter
        height: root.edgeDiameter
        radius: root.edgeReveal
        x: bridge.edgeSide === "left" ? -root.edgeReveal : bridge.edgeSide === "right" ? root.width - root.edgeReveal : (root.width - width) / 2
        y: bridge.edgeSide === "top" ? -root.edgeReveal : bridge.edgeSide === "bottom" ? root.height - root.edgeReveal : (root.height - height) / 2
        color: root.dark ? Qt.rgba(0.7, 0.9, 1, 0.82) : Qt.rgba(0.08, 0.2, 0.34, 0.74)
    }

    MouseArea {
        visible: bridge.edgeCollapsed
        x: edgeStrip.x
        y: edgeStrip.y
        width: edgeStrip.width
        height: edgeStrip.height
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onEntered: bridge.setEdgeCollapsed(false)
        onClicked: bridge.setEdgeCollapsed(false)
    }

    // 右键菜单：不抢焦点的小窗
    Window {
        id: menu
        objectName: "sysinfoOverlayMenu"
        color: "transparent"
        flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
        visible: false
        width: 208
        height: column.implicitHeight + 12
        title: ""

        function popupAt(globalX, globalY) {
            const area = bridge.availableArea()
            x = Math.round(Math.min(globalX, area.right - width - 8))
            y = Math.round(Math.min(globalY, area.bottom - height - 8))
            show()
        }

        Rectangle {
            anchors.fill: parent
            radius: 12
            color: root.dark ? Qt.rgba(24 / 255, 26 / 255, 34 / 255, 0.92) : Qt.rgba(1, 1, 1, 0.95)
            border.width: 1
            border.color: root.dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0.06, 0.08, 0.10, 0.12)

            Column {
                id: column
                anchors.fill: parent
                anchors.margins: 6
                spacing: 1

                MenuRow {
                    text: qsTr("复制全部数值")
                    action: function () { bridge.copyAll() }
                }
                MenuRow {
                    text: bridge.paused ? qsTr("继续采样") : qsTr("暂停采样")
                    action: function () { bridge.setPaused(!bridge.paused) }
                }
                MenuRow {
                    text: bridge.position === "free" ? qsTr("恢复默认位置") : qsTr("锁定位置")
                    action: function () {
                        if (bridge.position === "free")
                            bridge.resetPosition()
                        else
                            bridge.edit("locked", !bridge.locked)
                    }
                }
                MenuRow {
                    text: bridge.clickThrough ? qsTr("恢复鼠标操作") : qsTr("鼠标穿透")
                    action: function () { bridge.edit("clickThrough", !bridge.clickThrough) }
                }
                MenuRow {
                    text: qsTr("隐藏监视窗")
                    action: function () { bridge.edit("enabled", false) }
                }
            }
        }

        onActiveChanged: if (!active) hide()
    }

    component MenuRow: Rectangle {
        id: menuRow
        required property string text
        required property var action
        width: column.width
        height: 30
        radius: 7
        color: menuHover.containsMouse
               ? (root.dark ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0.06, 0.08, 0.10, 0.07))
               : "transparent"
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 9
            text: menuRow.text
            color: root.dark ? "#f2f5fa" : "#111820"
            font.family: root.fontFamily
            font.pixelSize: 12
        }
        MouseArea {
            id: menuHover
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                menu.hide()
                menuRow.action()
            }
        }
    }

    Connections {
        target: bridge
        function onSettingsApplied() {
            if (!bridge.enabled)
                menu.hide()
            if (!bridge.edgeHide)
                bridge.setEdgeCollapsed(false)
        }
    }
}
