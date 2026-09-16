pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SudokuSolver

RowLayout {
    id: root

    required property SudokuBackend backend

    // Main.qml에서 결과창에 알고리즘 이름을 넘겨주기 위한 읽기 전용 프로퍼티
    readonly property string currentAlgorithmName: algorithmCombo.currentText

    Layout.alignment: Qt.AlignHCenter
    spacing: 12

    // 1. 알고리즘 선택 ComboBox
    ComboBox {
        id: algorithmCombo
        textRole: "text"
        valueRole: "value"
        model: [
            { text : "Backtracking", value: SudokuSolver.BackTracking },
            { text : "Randomly", value: SudokuSolver.Randomly },
            { text : "MRV", value: SudokuSolver.MRV }
        ]

        // C++ 백엔드 상태와 ComboBox 인덱스 동기화
        // 초기 로딩 타이밍 이슈로 -1이 나오면 기본값 0번 선택
        currentIndex: {
            let idx = indexOfValue(root.backend.algorithm);
            return idx >= 0 ? idx : 0;
        }
        enabled: !root.backend.isBusy
        Layout.preferredWidth: 130

        // 사용자가 직접 항목을 선택했을 때만 안전한 enum 값 대입
        onActivated: index => {
            root.backend.algorithm = currentValue;
        }
    }

    // 2. Solve / Pause / Resume 토글 버튼
    Button {
        text: !root.backend.isBusy ? "Solve" : (root.backend.isPaused ? "Resume" : "Pause")
        highlighted: !root.backend.isBusy
        enabled: root.backend.isBusy || !root.backend.hasErrors

        Layout.preferredWidth: 80

        onClicked: {
            if (root.backend.isBusy) {
                root.backend.togglePause();
            } else {
                root.backend.solve();
            }
        }
    }

    // 3. Stop 버튼
    Button {
        text: "Stop"
        opacity: root.backend.isBusy ? 1.0 : 0.0
        enabled: root.backend.isBusy
        Layout.preferredWidth: 80
        onClicked: root.backend.stop()
    }

    // 4. Generate 퍼즐 버튼
    RowLayout {
        spacing: 5
        enabled: !root.backend.isBusy

        ComboBox {
            id: difficultyCombo
            model: ["Easy", "Medium", "Hard"]
            currentIndex: 0
            width: 90
        }

        Button {
            text: "Generate"
            Layout.preferredWidth: 85
            onClicked: root.backend.generatePuzzle(difficultyCombo.currentIndex)
        }
    }

    // 5. Clear 버튼
    Button {
        text: "Clear"
        enabled: !root.backend.isBusy
        Layout.preferredWidth: 75
        onClicked: root.backend.clear()
    }
}
