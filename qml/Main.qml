import QtQuick
import QtQuick.Layouts
import SudokuSolver

pragma ComponentBehavior: Bound

Window {
    id: root
    width: 620
    height: 750
    visible: true
    title: qsTr("Sudoku Solver")

    // 백엔드 인스턴스
    SudokuBackend {
        id: sudokuBackend

        // 비동기로 전달되는 결과를 여기서 처리
        onSolveFinished: (status, elapsedMs) => {
            let algoName = controlBar.currentAlgorithmName;

            if (status === 0) {
                resultDialog.showReport(true, elapsedMs, algoName);
            } else if (status === 1) {
                resultDialog.showReport(false, 0, algoName);
            } else if (status === 2) {
                console.log("Solving was stopped by user.");
            }
        }
    }

    // ===================================================
    // 전체  화면을 그리는 메인 레이아웃
    // ===================================================
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // 상단 타이틀 표시
        Text {
            text: "Sudoku Solver"
            font.pixelSize: 28
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            color: "#2c3e50"
        }

        // 상단 상태 표시
        Text {
            text: sudokuBackend.isBusy ? "Solving puzzle..." : "Ready"
            font.pixelSize: 14
            color: sudokuBackend.isBusy ? "#e67e22" : "#27ae60"
            Layout.alignment: Qt.AlignHCenter
        }

        // 1. 스도쿠 그리드 (9x9)
        SudokuGrid {
            id: sudokuGrid
            backend: sudokuBackend
        }

        // 2. 실시간 알고리즘 탐색 가이드 바
        StepInfoBar {
            backend: sudokuBackend
        }

        // 3. 시각화 설정
        VisualizationSettings {
            backend: sudokuBackend
        }

        // 4. 제어 버튼
        ControlBar {
            id: controlBar
            backend: sudokuBackend
        }

        // 하단 여백
        Item {
            Layout.fillHeight: true
        }
    }

    // 5. 결과 리포트 다이얼로그
    ResultDialog {
        id: resultDialog
    }
}
