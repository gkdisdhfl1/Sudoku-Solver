import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SudokuSolver

pragma ComponentBehavior: Bound

Window {
    width: 620
    height: 750
    visible: true
    title: qsTr("Sudoku Solver")

    // 백엔드 인스턴스
    SudokuBackend {
        id: backend

        // 비동기로 전달되는 결과를 여기서 처리
        onSolveFinished: (status, elapsedMs) => {
            let algoName = algorithmCombo.currentText;

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

            // 특정 인덱스의 셀을 지정해 키보드 포커스를 강제하는 함수
            function focusCellAt(targetIndex) {
                // targetIndex에 대응하는 셀을 즉시 획득하여 존재할 시 포커스 전파
                cellRepeater.itemAt(targetIndex)?.focusInput?.();
            }

            Repeater {
                id: cellRepeater
                model: backend
                delegate: Rectangle {
                    id: cell

                    required property int index
                    required property int value // display 롤에 바인딩된 변수
                    required property bool isError
                    required property var candidates // C++ CandidatesRole 수신
                    required property bool isTarget // C++ IsTargetRole 수신

                    readonly property int row: Math.floor(index / 9)
                    readonly property int col: index % 9
                    readonly property int blockRow: Math.floor(row / 3)
                    readonly property int blockCol: Math.floor(col / 3)

                    // 3x3 구역 구분을 위한 배경색 교차 (체커보드 스타일)
                    readonly property bool isDarkBlock: (blockRow + blockCol) % 2 !== 0

                    implicitWidth: 48
                    implicitHeight: 48

                    // 타겟 셀일 때 연한 파란색 하이라이트
                    color: inputField.activeFocus
                        ? "#d6e4ff"
                        : (isError 
                            ? "#ffcccc" 
                            : (isDarkBlock ? "#ecf0f1" : "#ffffff"))

                    border.color: isTarget ? "#8e44ad" : "#bdc3c7"
                    border.width: isTarget ? 2 : 1

                    // ---------------------------------------------------
                    // 3x3 연필 자국 소형 노트 오버레이 (빈 셀일 때만 표시)
                    // ---------------------------------------------------
                    GridLayout {
                        anchors.fill: parent
                        anchors.margins: 2
                        columns: 3
                        rows: 3
                        visible: cell.value === 0 && backend.visualize

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

                    
                    // 외부에서 포커스 요청 시 텍스트 필드를 활성화하는 헬퍼
                    function focusInput() {
                        inputField.forceActiveFocus();
                        inputField.deselect(); // 드래그 지정(블록)을 강제로 해제
                    }

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
                        cursorDelegate: Item{}

                        // 1~9 정수만 입력 가능
                        validator: IntValidator {
                            bottom: 1
                            top: 9
                        }
                        inputMethodHints: Qt.ImhDigitsOnly

                        // 텍스트가 수정될 때 백엔드 업데이트
                        onTextEdited: {
                            let val = parseInt(text);
                            backend.setCell(cell.index, isNaN(val) ? 0 : val);
                        }
                        // ==========================================
                        // 숫자 입력 시 드래그 없이 즉각 덮어쓰기 및 지우기 처리
                        // ==========================================
                        // 1~9 숫자 및 Delete 키 입력 시 여기서 즉시 처리 후 event.accepted = true로 이벤트를 소비함
                        Keys.onPressed: (event) => {
                                            // 1~9 숫자 키 입력 시 덮어쓰기
                                            if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
                                                let num = event.key - Qt.Key_0;
                                                backend.setCell(cell.index, num);
                                                event.accepted = true;
                                            }
                                            // Delete, Backspace, 0 키 입력시 즉시 지우기
                                            else if (event.key === Qt.Key_Delete ||
                                                     event.key === Qt.Key_Backspace ||
                                                     event.key === Qt.Key_0) {
                                                backend.setCell(cell.index, 0);
                                                event.accepted = true;
                                            }
                                        }

                        // ==========================================
                        // 개별 방향키 전용 시그널 핸들러를 통한 캐럿 이동 무력화
                        // ==========================================
                        Keys.onLeftPressed: (event) => {
                                                if (cell.col > 0) {
                                                    sudokuGrid.focusCellAt(cell.index - 1);
                                                    event.accepted = true; // 기본 커서 이동 동작 차단 및 소모
                                                }
                                            }

                        Keys.onRightPressed: (event) => {
                                                if (cell.col < 8) {
                                                    sudokuGrid.focusCellAt(cell.index + 1);
                                                    event.accepted = true; // 기본 커서 이동 동작 차단 및 소모
                                                }
                                            }

                        Keys.onUpPressed: (event) => {
                                                if (cell.row > 0) {
                                                    sudokuGrid.focusCellAt(cell.index - 9);
                                                    event.accepted = true; // 기본 커서 이동 동작 차단 및 소모
                                                }
                                            }

                        Keys.onDownPressed: (event) => {
                                                if (cell.row < 8) {
                                                    sudokuGrid.focusCellAt(cell.index + 9);
                                                    event.accepted = true; // 기본 커서 이동 동작 차단 및 소모
                                                }
                                            }
                    }

                    // ==========================================
                    // 3x3 구역 시각후 구분선 추가 데코레이터
                    // ==========================================

                    // 가로 3x3 블록 경계선 (2번째, 5번째 행의 바닥면에 두꺼운 선 추가)
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: (cell.row === 2 || cell.row === 5) ? 3: 0 // 3px 두께
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
            }
        }

        // 실시간 알고리즘 탐색 가이드 바
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.maximumWidth: 440
            Layout.preferredHeight: 36

            color: "#f8fafc"
            radius: 8
            border.color: "#e2e8f0"
            border.width: 1

            // MRV 실행 시 페이드인 처리
            visible: backend.isBusy && backend.statusMessage !== ""

            RowLayout {
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: "🔍 Step Info:"
                    font.pixelSize: 13
                    font.bold: true
                    color: "#8e44ad"
                }

                Text {
                    text: backend.statusMessage
                    font.pixelSize: 13
                    font.bold: true
                    color: "#2c3e50"
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
                    Text {
                        text: "Visualize"
                    }
                    Switch {
                        checked: backend.visualize
                        onToggled: backend.visualize = checked
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Speed"
                    }
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
            spacing: 12

            // 알고리즘 선택 ComboBox 신설
            ComboBox {
                id: algorithmCombo
                textRole: "text"
                valueRole: "value"
                model: [
                    { text : "Backtracking", value: SudokuSolver.Backtracking },
                    { text : "Randomly", value: SudokuSolver.Randomly },
                    { text : "MRV", value: SudokuSolver.MRV }
                ]

                // C++ 백엔드 상태와 ComboBox 인덱스 동기화
                // 초기 로딩 타이밍 이슈로 -1이 나오면 기본값 0번 선택
                currentIndex: {
                    let idx = indexOfValue(backend.algorithm);
                    return idx >= 0 ? idx : 0;
                }
                enabled: !backend.isBusy
                Layout.preferredWidth: 130

                // 사용자가 직접 항목을 선택했을 때만 안전한 enum 값 대입
                onActivated: (index) => {
                    backend.algorithm = currentValue;
                }
            }

            // Solve / Pause / Resume 토글 버튼
            Button {
                text: !backend.isBusy ? "Solve" : (backend.isPaused ? "Resume" : "Pause")
                highlighted: !backend.isBusy
                enabled: backend.isBusy || !backend.hasErrors

                Layout.preferredWidth: 80

                onClicked: {
                    if (backend.isBusy) {
                        backend.togglePause();
                    } else {
                        backend.solve();
                    }
                }
            }

            // Stop 버튼 (작업 중일 때만 보임)
            Button {
                text: "Stop"

                opacity: backend.isBusy? 1.0 : 0.0
                enabled: backend.isBusy

                Layout.preferredWidth: 80

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
                    width: 90
                }

                Button {
                    text: "Generate"
                    Layout.preferredWidth: 85
                    onClicked: backend.generatePuzzle(difficultyCombo.currentIndex)
                }
            }

            Button {
                text: "Clear"
                enabled: !backend.isBusy
                Layout.preferredWidth: 75
                onClicked: backend.clear()
            }
        }

        // 하단 여백
        Item {
            Layout.fillHeight: true
        }
    }

    // ===================================================
    // 풀이 통계 및 알고리즘 정보 출력을 위한 상세 리포트 다이얼로그
    // ===================================================
    Dialog {
        id: resultDialog
        
        // x, y 수식을 이용한 화면 중앙 정렬
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        
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
                    text: "Algorithm";
                    font.bold: true;
                    color: "#64748b"
                }
                Text {
                    id: valAlgorithm
                    text: "Backtracking" // 기본값 (추후 동적 바인딩)
                    font.bold: true
                    color: "#1e293b"
                }

                // 소요 시간
                Text {
                    text: "Time Elapsed";
                    font.bold: true;
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
                    text: "Status";
                    font.bold: true;
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
                    onClicked: resultDialog.close()
                }
            }
        }

        // 다이얼로그 연동 호출 함수 (추후 알고리즘 및 소요시간 변수 전달 가능)
        function showReport(isSuccess, elapsedMs = 0, algorithmName = "Backtracking") {
            reportIcon.text = isSuccess ? "🏆" : "⚠";
            reportTitle.text = isSuccess ? "Solved!" : "Unsolved";
            reportTitle.color = isSuccess ? "#2ecc71" : "#e74c3c"

            valAlgorithm.text = algorithmName;
            valTime.text = elapsedMs + " ms";

            valStatus.text = isSuccess ? "Success" : "No Solution";
            valStatus.color = isSuccess ? "#2ecc71" : "#e74c3c";

            resultDialog.open();
        }
    }
}
