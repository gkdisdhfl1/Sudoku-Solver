pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SudokuSolver

Rectangle {
    id: cell

    required property int index
    required property int value         // display 롤에 바인딩된 변수
    required property bool isError
    required property var candidates    // C++ CandidatesRole 수신
    required property bool isTarget     // C++ IsTargetRole 수신
    required property bool isGiven      // C++ isGivenRole 수신
    required property int stepState     // 0: None, 1: Selecting, 2: Trying, 3: DeadEnd, 4: Backtracking
    required property int tryingValue
    required property SudokuBackend backend
    required property var parentGrid

    readonly property int row: Math.floor(index / 9)
    readonly property int col: index % 9
    readonly property int blockRow: Math.floor(row / 3)
    readonly property int blockCol: Math.floor(col / 3)

    // 자신이 현재 포커스 대상인지 선언적 감지
    readonly property bool isFocusedCell: parentGrid.focusedIndex === cell.index
    readonly property bool hasCandidates: cell.candidates && cell.candidates.length > 0

    onIsFocusedCellChanged: {
        if (isFocusedCell) {
            inputField.forceActiveFocus();
            inputField.deselect();
        }
    }

    // 3x3 구역 구분을 위한 배경색 교차 (체커보드 스타일)
    readonly property bool isDarkBlock: (blockRow + blockCol) % 2 !== 0

    implicitWidth: 48
    implicitHeight: 48

    // 타겟 셀일 때 연한 파란색 하이라이트
    color: {
        if (cell.isTarget) {
            if (cell.stepState === 3) return "#fadbd8"; // DeadEnd: 연한 빨강
            if (cell.stepState === 4) return "#fdebd0"; // Backtracking: 연한 주황
            if (cell.stepState === 1) return "#e8f4fd"; // Selecting: 연한 하늘색
            return "#d6e4ff"
        }
        if (inputField.activeFocus) return "#d6e4ff"
        if (cell.isError) return "#ffcccc";
        return cell.isDarkBlock ? "#ecf0f1" : "#ffffff";
    }

    Behavior on color {
        ColorAnimation { duration : 90 }
    }

    border.color: {
        if (cell.isTarget) {
            if (cell.stepState === 3) return "#e74c3c"; // DeadEnd
            if (cell.stepState === 4) return "#e67e22"; // Backtracking
            return "#2980b9";                           // Selecting / Trying
        }
        return "#bdc3c7";
    }
    border.width: cell.isTarget ? 2.5 : 1

    // 1. 3x3 연필 자국 소형 노트 오버레이
    GridLayout {
        anchors.fill: parent
        anchors.margins: 3
        columns: 3
        rows: 3
        rowSpacing: 0
        columnSpacing: 0
        visible: cell.backend.visualize && cell.hasCandidates && (cell.value === 0 || cell.isTarget)
        opacity: (cell.value !== 0 && cell.isTarget) ? 0.38 : 1.0

        Repeater {
            model: [1, 2, 3, 4, 5, 6, 7, 8, 9]
            Text {
                required property int modelData
                readonly property int num: modelData
                readonly property bool isPossible: cell.candidates ? (cell.candidates.indexOf(num) !== -1) : false

                text: num.toString()
                opacity: isPossible ? 1.0 : 0.0
                font.pixelSize: cell.isTarget ? 10 : 9
                font.bold: cell.isTarget
                color: {
                    if (!cell.isTarget) return "#7f8c8d";
                    if (cell.tryingValue === num) {
                        return (cell.stepState === 4) ? "#c0392b" : "#d35400";
                    }
                    return "#1f618d";
                }
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                Layout.preferredHeight: 1
            }
        }
    }

    // [DeadEnd 시각화] 후보가 0개여서 막다른 길에 도달했을 때 경고 표시
    Text {
        anchors.centerIn: parent
        visible: cell.isTarget && cell.stepState === 3 && cell.value === 0
        text: "✕"
        font.pixelSize: 22
        font.bold: true
        color: "#e74c3c"
    }

    // 2. 숫자 입력 TextField & 키보드 탐색 제어
    TextField {
        id: inputField
        objectName: "inputField"
        anchors.fill: parent
        text: cell.value === 0 ? "" : cell.value.toString()
        font.pixelSize: 24
        font.bold: cell.isGiven || cell.isTarget
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        background: null
        selectByMouse: false
        color: {
            if (cell.isError) return "red";
            if (cell.isTarget && cell.stepState === 4) return "#c0392b";    // 백트래킹 실패 숫자
            if (cell.isTarget && cell.stepState === 2) return "#1d6fa5";    // 현재 대입 시도 숫자
            if (!cell.isGiven) return "#2471a3";                            // solver가 채운 숫자
            return "#2c3e50";                                               // 초기/사용자 숫자
        }

        // 커서를 그리는 델리게이트를 빈 아이템으로 오버라이드하여 제거
        cursorDelegate: Item {}

        // 1~9 정수만 입력 가능
        validator: IntValidator {
            bottom: 1
            top: 9
        }
        inputMethodHints: Qt.ImhDigitsOnly

        // 텍스트가 수정될 때 백엔드 업데이트
        onTextEdited: {
            let val = parseInt(text);
            cell.backend.setCell(cell.index, isNaN(val) ? 0 : val);
        }

        // 마우스로 셀을 클릭했을 때도 focusedIndex를 즉시 동기화
        onActiveFocusChanged: {
            if (activeFocus && cell.parentGrid.focusedIndex !== cell.index) {
                cell.parentGrid.focusedIndex = cell.index;
            }
        }

        // ==========================================
        // 숫자 입력 시 드래그 없이 즉각 덮어쓰기 및 지우기 처리
        // ==========================================
        // 1~9 숫자 및 Delete 키 입력 시 여기서 즉시 처리 후 event.accepted = true로 이벤트를 소비함
        Keys.onPressed: event => {
            // 1~9 숫자 키 입력 시 덮어쓰기
            if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
                let num = event.key - Qt.Key_0;
                cell.backend.setCell(cell.index, num);
                event.accepted = true;
            } else
            // Delete, Backspace, 0 키 입력시 즉시 지우기
            if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace || event.key === Qt.Key_0) {
                cell.backend.setCell(cell.index, 0);
                event.accepted = true;
            }
        }

        // ==========================================
        // 개별 방향키 전용 시그널 핸들러를 통한 캐럿 이동 무력화
        // ==========================================
        Keys.onLeftPressed: event => {
            if (cell.col > 0) {
                cell.parentGrid.focusCellAt(cell.index - 1);
                event.accepted = true; // 기본 커서 이동 동작 차단 및 소모
            }
        }

        Keys.onRightPressed: event => {
            if (cell.col < 8) {
                cell.parentGrid.focusCellAt(cell.index + 1);
                event.accepted = true; // 기본 커서 이동 동작 차단 및 소모
            }
        }

        Keys.onUpPressed: event => {
            if (cell.row > 0) {
                cell.parentGrid.focusCellAt(cell.index - 9);
                event.accepted = true; // 기본 커서 이동 동작 차단 및 소모
            }
        }

        Keys.onDownPressed: event => {
            if (cell.row < 8) {
                cell.parentGrid.focusCellAt(cell.index + 9);
                event.accepted = true; // 기본 커서 이동 동작 차단 및 소모
            }
        }
    }

    // 3. 3x3 구역 시각후 구분선 추가 데코레이터
    // 가로 3x3 블록 경계선 (2번째, 5번째 행의 바닥면에 두꺼운 선 추가)
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: (cell.row === 2 || cell.row === 5) ? 3 : 0 // 3px 두께
        color: "#34495e" // 판 전체와 대비되는 짙은 차콜 색상
        visible: cell.row < 8 // 최하단 바깥 경계는 윈도우 테두리가 있어 제외
    }

    // 세로 3x3 블록 경계선 (2번째, 5번째 열의 바닥면에 두꺼운 선 추가)
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: (cell.col === 2 || cell.col === 5) ? 3 : 0 // 3px 두께
        color: "#34495e" // 판 전체와 대비되는 짙은 차콜 색상
        visible: cell.col < 8 // 최우측 바깥 경계는 윈도우 테두리가 있어 제외
    }
}
