pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import SudokuSolver

Rectangle {
    id: root

    required property SudokuBackend backend

    Layout.alignment: Qt.AlignHCenter
    Layout.fillWidth: true
    Layout.maximumWidth: 550
    Layout.preferredHeight: 42

    readonly property int stepState: root.backend.stepState
    readonly property bool isActive: root.backend.isBusy && root.backend.visualize && root.backend.targetRow >= 0

    color: {
        if (!root.isActive) return "#f4f6f7";
        if (root.stepState === 3) return "#fdedec"; // DeadEnd
        if (root.stepState === 4) return "#fef5e7"; // Backtracking
        return "#eaf2f8";                           // Selecting / Trying
    }
    radius: 8
    border.color: {
        if (!root.isActive) return "#dce1e5";
        if (root.stepState === 3) return "#e74c3c";
        if (root.stepState === 4) return "#f39c12";
        return "#85c1e9";
    }
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        // 1. 단계 상태 배지
        Rectangle {
            visible: root.isActive
            Layout.preferredHeight: 24
            Layout.preferredWidth: badgeText.implicitWidth + 16
            radius: 4
            color: {
                if (root.stepState === 1) return "#2980b9"; // 후보 탐색
                if (root.stepState === 2) return "#27ae60"; // 대입 시도
                if (root.stepState === 3) return "#c0392b"; // 모순 감지
                
                if (root.stepState === 4) return "#d35400"; // 백트래킹
                
                return "#7f8c8d";
            }
            Text {
                id: badgeText
                anchors.centerIn: parent
                font.pixelSize: 11
                font.bold: true
                color: "#ffffff"
                text: {
                    if (root.stepState === 1) return "후보 탐색";
                    if (root.stepState === 2) return "대입 시도";
                    if (root.stepState === 3) return "모순 감지!";
                    if (root.stepState === 4) return "백트래킹";
                    return "진행 중";
                }
            }
        }

        // 타겟 셀 좌표 및 현재 액션 설명
        Text {
            Layout.fillWidth: true
            elide: Text.ElideRight
            font.pixelSize: 12
            font.bold: true
            color: {
                if (!root.isActive) return "#7f8c8d";
                if (root.stepState === 3) return "#c0392b";
                if (root.stepState === 4) return "#d35400";
                return "#2c3e50";
            }
            text: {
                if (!root.isActive) {
                    return root.backend.statusMessage !== "" ? root.backend.statusMessage : "Visualize 대기 중 (풀이 시작 시 탐색 과정이 표시됩니다)";
                }

                const r = root.backend.targetRow + 1;
                const c = root.backend.targetCol + 1;
                if (root.stepState === 1) return `Cell (${r}, ${c}) 최소 후보 셀 선택`;
                if (root.stepState === 2) return `Cell (${r}, ${c}) ⟵ '${root.backend.tryingValue}'대입`;
                if (root.stepState === 3) return `Cell (${r}, ${c}) 가능한 후보 없음! (되돌아감)`;
                if (root.stepState === 4) return `Cell (${r}, ${c}) ✕ '${root.backend.tryingValue}'실패 (백트래킹)`;
                return root.backend.statusMessage;
            }
        }

        // 3. 고정 위치 1~9 후보 
        RowLayout {
            visible: root.isActive && root.backend.targetCandidates.length > 0
            spacing: 3

            Repeater {
                model: [1, 2, 3, 4, 5, 6, 7, 8, 9]
                Rectangle {
                    id: chip
                    required property int modelData
                    readonly property bool isCand: root.backend.targetCandidates.indexOf(modelData) !== -1
                    readonly property bool isCurrent: root.backend.tryingValue === modelData

                    Layout.preferredHeight: 22
                    Layout.preferredWidth: 18
                    radius: 4
                    color: {
                        if (isCurrent) return (root.stepState === 4) ? "#e74c3c" : "#2980b9";
                        if (isCand) return "#ffffff";
                        return "transparent";
                    }
                    border.color: isCurrent ? "transparent" : (isCand ? "#85c1e9" : "#dce1e5")
                    border.width: isCand ? 1 : 0.5
                    opacity: isCand ? 1.0 : 0.25

                    Text {
                        anchors.centerIn: parent
                        text: chip.modelData.toString()
                        font.pixelSize: 11
                        font.bold: chip.isCurrent || chip.isCand
                        color: chip.isCurrent ? "#ffffff" : (chip.isCand ? "#1f618d" : "#95a5a6")
                    }
                }
            }
        }
    }
}
