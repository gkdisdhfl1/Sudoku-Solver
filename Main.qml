import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SudokuSolver

Window {
    width: 550
    height: 750
    visible: true
    title: qsTr("Sudoku Solver")

    // 백엔드 인스턴스
    SudokuBackend {
        id: backend
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Text {
            text: "Sudoku Solver"
            font.pixelSize: 28
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            color: "#2c3e50"
        }

        // --- 상단 상태 표시 ---
        Text {
            text: backend.isBusy ? "Solving puzzle..." : "Ready"
            font.pixelSize: 14
            color: backend.isBusy ? "#e67e22" : "#27ae60"
            Layout.alignment: Qt.AlignHCenter
        }

        // 스도쿠 그리드 (9x9)
        GridLayout {
            id: sudokuGrid
            columns: 9
            rowSpacing: 0
            columnSpacing: 0
            Layout.alignment: Qt.AlignHCenter

            // 작업 중일 때 그리드 조작 방지
            enabled: !backend.isBusy

            Repeater {
                model: backend.board
                delegate: Rectangle {
                    readonly property int row: Math.floor(index / 9)
                    readonly property int col: index % 9
                    readonly property int blockRow: Math.floor(row / 3)
                    readonly property int blockCol: Math.floor(col / 3)

                    // 3x3 구역 구분을 위한 배경색 교차 (체커보드 스타일)
                    readonly property bool isDarkBlock: (blockRow + blockCol) % 2 !== 0

                    // 백엔드의 errorCells 리스트에 현재 인덱스가 포함되어 있는지  확인
                    readonly property bool isError: backend.errorCells.includes(index);

                    implicitWidth: 48
                    implicitHeight: 48

                    color: isError ? "#ffcccc" : (isDarkBlock ? "#ecf0f1" : "#ffffff")
                    border.color: "#bdc3c7"
                    border.width: 1

                    TextField {
                        anchors.fill: parent
                        text: modelData === 0 ? "" : modelData.toString()
                        font.pixelSize: 24
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        background: null
                        selectByMouse: true
                        color: isError ? "red" : "black" // 에러 시 글자색 빨강

                        // 1~9 정수만 입력 가능
                        validator: IntValidator { bottom: 1; top: 9 }
                        inputMethodHints: Qt.ImhDigitsOnly

                        // 텍스트가 수정될 때 백엔드 업데이트
                        onTextEdited: {
                            let val = parseInt(text);
                            backend.setCell(index, isNaN(val) ? 0 : val);
                        }
                    }

                    // 3x3 구역 경계선 더 명확하게 하고 싶으면  추가 스타일링
                }
            }
        }

        // --- 시각화 설정 ---
        GroupBox {
            title: "Visualization Settings"
            Layout.fillWidth: true
            enabled: !backend.isBusy // 실행 중엔 설정 변경 금지

            RowLayout {
                anchors.fill: parent
                spacing: 20

                RowLayout {
                    Text { text: "Visualize" }
                    Switch {
                        checked: backend.visualize
                        onToggled: backend.visualize = checked
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Speed" }
                    Slider {
                        id: delaySlider
                        Layout.fillWidth: true
                        from: 1
                        to: 500
                        value: backend.delay
                        onMoved: backend.delay = value
                    }
                    Text {
                        text: Math.floor(delaySlider.value) + "ms"
                        Layout.preferredWidth: 40
                    }
                }
            }
        }

        // 제어 버튼
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 15

            // Solve / Pause / Resume 토글 버튼
            Button {
                text: !backend.isBusy ? "Solve" : (backend.isPaused ? "Resume" : "Pause")
                highlighted: !backend.isBusy
                enabled: backend.isBusy || backend.isValidBoard()

                onClicked: {
                    if(backend.isBusy) {
                        backend.togglePause();
                    } else {
                        // 1. 유효성 검사
                        if(!backend.isValidBoard()) {
                            console.log("Invalid board configuration!");
                            // todo: 나중에 팝업 등으로 사용자에게 알림
                            return;
                        }

                        // 2. 풀이 시도
                        if(!backend.solveBacktracking()) {
                            console.log("Solution not found");
                        }
                    }
                }
            }

            // Stop 버튼 (작업 중일 때만 보임)
            Button {
                text: "Stop"
                visible: backend.isBusy
                onClicked: backend.stop()
            }

            // Generate 퍼즐 버튼
            RowLayout {
                spacing: 5
                enabled: !backend.isBusy

                ComboBox {
                    id: difficultyCombo
                    model: ["Easy", "Medium", "Hard"]
                    currentIndex: 0
                    width: 100
                }

                Button {
                    text: "Generate"
                    onClicked: backend.generatePuzzle(difficultyCombo.currentIndex);
                }
            }

            Button {
                text: "Clear"
                enabled: !backend.isBusy
                onClicked: backend.clear()
            }
        }

        // 하단 여백
        Item {
            Layout.fillHeight: true
        }
    }
}
