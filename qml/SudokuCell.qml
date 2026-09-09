pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SudokuSolver

Rectangle {
    id: cell

    required property int index
    required property int value // display 롤에 바인딩된 변수
    required property bool isError
    required property var candidates // C++ CandidatesRole 수신
    required property bool isTarget // C++ IsTargetRole 수신
    required property SudokuBackend backend
    required property var parentGrid

    readonly property int row: Math.floor(index / 9)
    readonly property int col: index % 9
    readonly property int blockRow: Math.floor(row / 3)
    readonly property int blockCol: Math.floor(col / 3)

    // 자신이 현재 포커스 대상인지 선언적 감지
    readonly property bool isFocusedCell: parentGrid.focusedIndex === cell.index

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
    color: inputField.activeFocus ? "#d6e4ff" : (isError ? "#ffcccc" : (isDarkBlock ? "#ecf0f1" : "#ffffff"))

    border.color: isTarget ? "#8e44ad" : "#bdc3c7"
    border.width: isTarget ? 2 : 1

    // 1. 3x3 연필 자국 소형 노트 오버레이
    GridLayout {
        anchors.fill: parent
        anchors.margins: 2
        columns: 3
        rows: 3
        visible: cell.value === 0 && cell.backend.visualize

        Repeater {
            model: 9 // 1~9 위치
            Text {
                required property int index
                readonly property int num: index + 1
                readonly property bool isPossible: cell.candidates ? cell.candidates.includes(num) : false

                text: isPossible ? num.toString() : ""
                font.pixelSize: 9
                font.bold: cell.isTarget
                color: cell.isTarget ? "#8e44ad" : "#94a3b8" // 타겟이면 진한 보라색
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    // 2. 숫자 입력 TextField & 키보드 탐색 제어
    TextField {
        id: inputField
        anchors.fill: parent
        text: cell.value === 0 ? "" : cell.value.toString()
        font.pixelSize: 24
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        background: null
        selectByMouse: false
        color: cell.isError ? "red" : "black" // 에러 시 글자색 빨강

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
