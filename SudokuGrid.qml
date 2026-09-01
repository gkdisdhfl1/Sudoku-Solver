pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import SudokuSolver

GridLayout {
    id: grid

    required property SudokuBackend backend

    // 현재 포커스 대상 인덱스
    property int focusedIndex: -1

    columns: 9
    rowSpacing: 0
    columnSpacing: 0
    Layout.alignment: Qt.AlignHCenter

    // 작업 중일 때 그리드 조작 방지
    enabled: !backend.isBusy

    // 포커스 셀 상태값 갱신
    function focusCellAt(targetIndex) {
        focusedIndex = targetIndex;
    }

    Repeater {
        id: cellRepeater
        model: grid.backend
        delegate: SudokuCell {
            backend: grid.backend
            parentGrid: grid
        }
    }
}
