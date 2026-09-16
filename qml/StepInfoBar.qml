pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import SudokuSolver

Rectangle {
    id: root

    required property SudokuBackend backend

    Layout.alignment: Qt.AlignHCenter
    Layout.fillWidth: true
    Layout.maximumWidth: 440
    Layout.preferredHeight: 36

    color: "#f8fafc"
    radius: 8
    border.color: "#e2e8f0"
    border.width: 1

    // MRV 실행 시 페이드인 처리
    visible: root.backend.isBusy && root.backend.statusMessage !== ""

    RowLayout {
        anchors.centerIn: parent
        spacing: 8

        Text {
            text: "🔍 Step Info:"
            font.pixelSize: 13
            font.bold: true
            color: "#8e44ad"
        }

        Text {
            text: root.backend.statusMessage
            font.pixelSize: 13
            font.bold: true
            color: "#2c3e50"
        }
    }
}
