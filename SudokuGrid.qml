import QtQuick
import QtQuick.Layouts
import SudokuSolver

GridLayout {
    id: grid

    required property SudokuBackend backend

    columns: 9
    rowSpacing: 0
    columnSpacing: 0
    Layout.alignment: Qt.AlignHCenter

    // 작업 중일 때 그리드 조작 방지
    enabled: !backend.isBusy

    // 특정 인덱스의 셀을 지정해 키보드 포커스를 강제하는 함수
    function focusCellAt(targetIndex) {
        // targetIndex에 대응하는 셀을 즉시 획득하여 존재할 시 포커스 전파
        cellRepeater.itemAt(targetIndex)?.focusInput?.();
    }

    Repeater {
        id: cellRepeater
        model: grid.backend
        delegate: SudokuCell {
            backend: grid.backend
            grid: grid
        }
    }
}