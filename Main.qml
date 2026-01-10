import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SudokuSolver

Window {
    width: 500
    height: 650
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

        // 스도쿠 그리드 (9x9)
        GridLayout {
            id: sudokuGrid
            columns: 9
            rowSpacing: 0
            columnSpacing: 0
            Layout.alignment: Qt.AlignHCenter

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

                    implicitWidth: 45
                    implicitHeight: 45

                    color: isError ? "#ffcccc" : (isDarkBlock ? "#ecf0f1" : "#ffffff")
                    border.color: "#bdc3c7"
                    border.width: 1

                    TextField {
                        anchors.fill: parent
                        text: modelData === 0 ? "" : modelData.toString()
                        font.pixelSize: 22
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

        // 제어 버튼
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 15

            Button {
                text: "Solve"
                highlighted: true
                onClicked: {
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

            Button {
                text: "Clear"
                onClicked: backend.clear()
            }
        }

        // 하단 여백
        Item {
            Layout.fillHeight: true
        }
    }
}
