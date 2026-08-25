pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    // x, y 수식을 이용한 화면 중앙 정렬
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    width: 320
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: "#ffffff"
        radius: 16
        border.color: "#e2e8f0"
        border.width: 1
        layer.enabled: true
    }

    // 다이얼로그 내부 레이아웃
    ColumnLayout {
        width: parent.width
        spacing: 20
        anchors.margins: 10

        // 1. 헤더 (성공/실패 비주얼)
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10
            Text {
                id: reportIcon
                font.pixelSize: 24
            }
            Text {
                id: reportTitle
                font.pixelSize: 20
                font.bold: true
            }
        }

        // 구분선
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#f1f5f9"
        }

        // 2. 핵심 분석 메타데이터 테이블 (알고리즘, 소요시간 등)
        GridLayout {
            columns: 2
            columnSpacing: 25
            rowSpacing: 12
            Layout.alignment: Qt.AlignHCenter

            // 알고리즘 종류
            Text {
                text: "Algorithm"
                font.bold: true
                color: "#64748b"
            }
            Text {
                id: valAlgorithm
                text: "Backtracking"
                font.bold: true
                color: "#1e293b"
            }

            // 소요 시간
            Text {
                text: "Time Elapsed"
                font.bold: true
                color: "#64748b"
            }
            Text {
                id: valTime
                text: "0 ms" // 기본값
                font.bold: true
                color: "#1e293b"
            }

            // 상태 메시지
            Text {
                text: "Status"
                font.bold: true
                color: "#64748b"
            }
            Text {
                id: valStatus
                font.bold: true
            }
        }

        // 구분선
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#f1f5f9"
        }

        // 3. 닫기 버튼
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 120
            height: 36
            color: mouseArea.pressed ? "#2c3e50" : "#34495e"
            radius: 8

            Text {
                anchors.centerIn: parent
                text: "Confirm"
                color: "white"
                font.bold: true
                font.pixelSize: 14
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.close()
            }
        }
    }

    // 다이얼로그 연동 호출 함수
    function showReport(isSuccess, elapsedMs = 0, algorithmName = "Backtracking") {
        reportIcon.text = isSuccess ? "🏆" : "⚠";
        reportTitle.text = isSuccess ? "Solved!" : "Unsolved";
        reportTitle.color = isSuccess ? "#2ecc71" : "#e74c3c";

        valAlgorithm.text = algorithmName;
        valTime.text = elapsedMs + " ms";

        valStatus.text = isSuccess ? "Success" : "No Solution";
        valStatus.color = isSuccess ? "#2ecc71" : "#e74c3c";

        root.open();
    }
}
