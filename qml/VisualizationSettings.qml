pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SudokuSolver

GroupBox {
    id: root

    required property SudokuBackend backend

    title: "Visualization Settings"
    Layout.fillWidth: true
    enabled: !root.backend.isBusy // 실행 중엔 설정 변경 금지

    RowLayout {
        anchors.fill: parent
        spacing: 20

        RowLayout {
            Text {
                text: "Visualize"
            }
            Switch {
                checked: root.backend.visualize
                onToggled: root.backend.visualize = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Speed"
            }
            Slider {
                id: delaySlider
                Layout.fillWidth: true
                from: 1
                to: 500
                value: root.backend.delay
                onMoved: root.backend.delay = value
            }
            Text {
                text: Math.floor(delaySlider.value) + "ms"
                Layout.preferredWidth: 40
            }
        }
    }
}
