#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include "sudoku_driver.h"
#include "sudoku_backend.h"

class TestSudokuGui : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init(); // 각 테스트 시작 전 백엔드 API로 보드/UI 상태를 완전 초기화

    // 1. 숫자 클릭, 덮어쓰기 및 지우기 무결성 검증
    void testCellClickAndTypeOverwrite();

    // 2. 방향키 네비게이션 및 경계 조건 검증
    void testArrowKeyNavigation();

    // 3. 모델 리셋(clear) 후에도 이전에 얻은 CellDriver가 새 델리게이트를 가리키는지 검증
    void testCellDriverSurvivesModelReset();

    // 4. 중복 숫자 입력 시 isError() 하이라이트 발생 및 삭제 시 해제 검증
    void testConflictHighlightsError();

    // 5. 셀 값 입력 시 동일 행/열/블록 피어 셀들의 candidates() 실시간 동기화 검증
    void testPeerCandidatesSyncOnInput();

    // 6. 백엔드가 작업 중(isBusy)일 때 UI 조작 방지(enabled: false) 검증
    void testGridDisabledWhileBusy();

private:
    bool m_defaultVisualize{false};
    int m_defaultDelay{0};

    SudokuBackend* findBackend() const;
    void resetBoard();

    std::unique_ptr<QQmlApplicationEngine> m_engine;
    QQuickWindow* m_window{nullptr};
};

SudokuBackend* TestSudokuGui::findBackend() const
{
    auto* grid = m_window->findChild<QQuickItem*>(QStringLiteral("sudokuGrid"));
    return grid ? qvariant_cast<SudokuBackend*>(grid->property("backend")) : nullptr;
}

void TestSudokuGui::initTestCase()
{
    QTest::setThrowOnFail(true); // 드라이버 헬퍼의 QVERIFY 실패 시 테스트 함수 즉시 중단
    m_engine = std::make_unique<QQmlApplicationEngine>();
    m_engine->loadFromModule("SudokuSolver", "Main");

    const auto rootObjects{m_engine->rootObjects()};
    QVERIFY2(!rootObjects.isEmpty(), "Main.qml failed to load!");

    m_window = qobject_cast<QQuickWindow*>(rootObjects.first());
    QVERIFY2(m_window != nullptr, "Root object is not a QQuickWindow!");

    auto* repeater = sudoku_test::findRepeater(m_window);
    QVERIFY2(repeater, "sudokuGrid/cellRepeater objectName not found");
    QTRY_COMPARE(repeater->property("count").toInt(), 81); // 델리게이트 수 검증

    SudokuDriver driver(m_window);
    QVERIFY2(driver.cell(0).exists() && driver.cell(80).exists(), "Cell lookup failed (itemAt 경롸 확인)");

    SudokuBackend* backend = findBackend();
    QVERIFY(backend);
    m_defaultVisualize = backend->visualize();
    m_defaultDelay = backend->delay();

    m_window->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    m_window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(m_window));
}

void TestSudokuGui::cleanupTestCase()
{
    if (m_window) {
        m_window->close();
    }
    m_engine.reset();
}

void TestSudokuGui::init()
{
    if (m_window && !m_window->isActive()) {
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(m_window));
    }
    resetBoard();
}

void TestSudokuGui::testCellClickAndTypeOverwrite()
{
    SudokuDriver driver(m_window);

    // 1. (0, 0) 셀 클릭 후 숫자 '5' 입력
    driver.cell(0, 0).click().type("5");
    QTRY_COMPARE(driver.cell(0, 0).value(), 5);

    // 2. 이미 숫자가 채워진 상태에서 숫자 '9' 입력 -> 백스페이스 없이 즉시 '9'로 덮어써져야 함
    driver.cell(0, 0).type("9");
    QTRY_COMPARE(driver.cell(0, 0).value(), 9);

    // 3. Backspace 키 입력 -> 즉시 0(빈 칸)으로 지워져야 함
    driver.cell(0, 0).press(Qt::Key_Backspace);
    QTRY_COMPARE(driver.cell(0, 0).value(), 0);

    // 4. 숫자 '3' 입력 후 '0' 키 입력 -> 지워져야 함
    driver.cell(0, 0).type("3");
    QTRY_COMPARE(driver.cell(0, 0).value(), 3);
    driver.cell(0, 0).type("0");
    QTRY_COMPARE(driver.cell(0, 0).value(), 0);
}

void TestSudokuGui::testArrowKeyNavigation()
{
    SudokuDriver driver(m_window);

    // 1. (0, 0) 셀을 클릭하여 포커스 획득
    driver.cell(0, 0).click();
    QTRY_VERIFY(driver.cell(0, 0).isFocused());

    // 2. 오른쪽 방향키 -> (0, 1)로 포커스 이동
    driver.press(Qt::Key_Right);
    QTRY_VERIFY(driver.cell(0, 1).isFocused());

    // 3. 아래쪽 방향키 -> (1, 1)로 포커스 이동
    driver.press(Qt::Key_Down);
    QTRY_VERIFY(driver.cell(1, 1).isFocused());

    // 4. 왼쪽 방향키 -> (1, 0)로 포커스 이동
    driver.press(Qt::Key_Left);
    QTRY_VERIFY(driver.cell(1, 0).isFocused());

    // 5. 위쪽 방향키 -> (0, 0)로 복귀
    driver.press(Qt::Key_Up);
    QTRY_VERIFY(driver.cell(0, 0).isFocused());

    // 6. 경계 조건 검증
    // 최상단 (0, 0)에서 위/왼쪽 키를 눌러도 보드 밖으로 튀지 않고 (0, 0) 유지
    driver.press(Qt::Key_Left);
    QTRY_VERIFY(driver.cell(0, 0).isFocused());
    driver.press(Qt::Key_Up);
    QTRY_VERIFY(driver.cell(0, 0).isFocused());

    // 최하단(8, 8)에서 아래/오른쪽 키를 눌러도 보드 밖으로 튀지 않고 (8, 8) 유지
    driver.cell(8, 8).click();
    QTRY_VERIFY(driver.cell(8, 8).isFocused());
    driver.press(Qt::Key_Right);
    QTRY_VERIFY(driver.cell(8, 8).isFocused());
    driver.press(Qt::Key_Down);
    QTRY_VERIFY(driver.cell(8, 8).isFocused());
}

void TestSudokuGui::testCellDriverSurvivesModelReset()
{
    SudokuBackend* backend = findBackend();
    QVERIFY2(backend, "SudokuGrid.backend not found");

    // 리셋 이전에 핸들을 얻어 둔다 (임시 SudokuDriver에서 받은 값 복사 핸들)
    CellDriver cell{SudokuDriver::on(m_window).cell(0, 0)};
    CellDriver neighbor{cell.cell(1, 1)}; // 체이닝으로 얻은 핸들도 함께 검증

    cell.click().type("5");
    QCOMPARE(cell.value(), 5);

    // 리셋 전 델리게이트를 약한 참조로 보관 (파괴 완료를 동기화 조건으로 사용)
    QPointer<QQuickItem> oldCell = cell.item();
    QPointer<QQuickItem> oldNeighbor = neighbor.item();

    // Repeater가 델리게이트를 모두 파괴/재생성하도록 모델 리셋 유발
    backend->clear();

    // 이벤트 루프를 돌리기 전에도 같은 핸들이 '새' 델리게이트를 가리켜야 함
    auto* repeater{sudoku_test::findRepeater(m_window)};
    QCOMPARE(cell.item(), sudoku_test::fetchCellFromRepeater(repeater, 0));
    QCOMPARE(cell.value(), 0);
    QCOMPARE(neighbor.value(), 0);

    // 옛 델리게이트가 실제로 파괴될 때까지 이벤트 루프를 돌림 (deleteLater 처리)
    QTRY_VERIFY(oldCell.isNull());
    QTRY_VERIFY(oldNeighbor.isNull());

    // 같은 핸들이 새 델리게이트를 읽어야 한다 (값은 초기화되어 0)
    QTRY_COMPARE(cell.value(), 0);
    QTRY_COMPARE(neighbor.value(), 0);
    QVERIFY(!cell.isError());

    // 새 델리게이트에 대해 조작도 가능해야 한다
    cell.click().type("7");
    QCOMPARE(cell.value(), 7);
    QVERIFY(cell.isFocused());

    backend->clear(); // 뒷정리
}

void TestSudokuGui::testConflictHighlightsError()
{
    SudokuDriver driver(m_window);

    // 1. (0, 0)에 5 입력 -> 단독 입력 시 에러 없음
    driver.cell(0, 0).click().type("5");
    QTRY_COMPARE(driver.cell(0, 0).value(), 5);
    QVERIFY(!driver.cell(0, 0).isError());

    // 2. 같은 행의 (0, 1)에 동일한 숫자 5 입력 -> 충돌 발생으로 두 셀 모두 isError() == true
    driver.cell(0, 1).click().type("5");
    QTRY_COMPARE(driver.cell(0, 1).value(), 5);
    QVERIFY(driver.cell(0, 0).isError());
    QVERIFY(driver.cell(0, 1).isError());

    // 3. (0, 1)의 숫자를 지움 -> 충돌 해소로 두 셀 모두 isError() == false 원복
    driver.cell(0, 1).press(Qt::Key_Backspace);
    QTRY_COMPARE(driver.cell(0, 1).value(), 0);
    QVERIFY(!driver.cell(0, 0).isError());
    QVERIFY(!driver.cell(0, 1).isError());
}

void TestSudokuGui::testPeerCandidatesSyncOnInput()
{
    SudokuDriver driver(m_window);

    // 1. 초기 상태: 모든 빈 셀은 1~9 후보를 가짐 (숫자 5 포함 확인)
    QVERIFY(driver.cell(0, 1).candidates().contains(5)); // 같은 행
    QVERIFY(driver.cell(1, 0).candidates().contains(5)); // 같은 열
    QVERIFY(driver.cell(1, 1).candidates().contains(5)); // 같은 3x3 블록
    QVERIFY(driver.cell(8, 8).candidates().contains(5)); // 무관한 비피어 셀

    // 2. (0, 0)에 5 입력
    driver.cell(0, 0).click().type("5");
    QTRY_COMPARE(driver.cell(0, 0).value(), 5);

    // 3. 피어 셀들의 candidates()에서 숫자 5가 즉시 제외되었는지 검증
    QTRY_VERIFY(!driver.cell(0, 1).candidates().contains(5)); // 같은 행
    QTRY_VERIFY(!driver.cell(1, 0).candidates().contains(5)); // 같은 열
    QTRY_VERIFY(!driver.cell(1, 1).candidates().contains(5)); // 같은 3x3 블록

    // 4. 대조군: 행/열/블록이 무관한 (8, 8) 셀은 여전히 5를 후보로 유지해야 함
    QVERIFY(driver.cell(8, 8).candidates().contains(5));

    // 5. (0, 0)의 5를 지웠을 때 피어들의 후보 5가 다시 복원되는지 검증
    driver.cell(0, 0).press(Qt::Key_Backspace);
    QTRY_COMPARE(driver.cell(0, 0).value(), 0);
    QTRY_VERIFY(driver.cell(0, 1).candidates().contains(5)); // 같은 행
    QTRY_VERIFY(driver.cell(1, 0).candidates().contains(5)); // 같은 열
    QTRY_VERIFY(driver.cell(1, 1).candidates().contains(5)); // 같은 3x3 블록
}

void TestSudokuGui::testGridDisabledWhileBusy()
{
    SudokuBackend* backend = findBackend();
    QVERIFY2(backend, "SudokuGrid.backend not found");

    auto* grid = m_window->findChild<QQuickItem*>(QStringLiteral("sudokuGrid"));
    QVERIFY2(grid, "sudokuGrid not found");

    SudokuDriver driver(m_window);

    // 1. 시각화 지연(500ms) 설정 후 풀이 시작하여 isBusy 상태 지속 유도
    backend->setVisualize(true);
    backend->setDelay(500);
    backend->solve();

    QTRY_VERIFY(backend->isBusy());

    // 2. SudokuGrid의 enabled 프로퍼티가 false로 비활성화되었는지 검증
    QTRY_COMPARE(grid->property("enabled").toBool(), false);

    // 3. 실제 GUI 입력 시도 -> 비활성 그리드는 포커스/입력을 받지 않아야 함
    //    click()은 포커스 획득을 기다리며 단언하므로 원시 이벤트를 사용
    QQuickItem* cell80 {driver.cell(80).item()};
    QPointF center {cell80->width() / 2.0, cell80->height() / 2.0};
    QPointF scenePos {cell80->mapToScene(center)};
    QTest::mouseClick(m_window, Qt::LeftButton, Qt::NoModifier, scenePos.toPoint());
    QVERIFY(!driver.cell(80).isFocused());

    // 4. 포커스가 없으므로 키보드를 눌러도 셀 값에 아무런 영향이 없어야 함
    QTest::keyClick(m_window, '9');
    QCOMPARE(driver.cell(80).value(), 0);
}

// 앞 테스트(실패 포함)의 상태가 남지 않도록 보드 전체와 UI 상태를 초기화
void TestSudokuGui::resetBoard()
{
    SudokuBackend* backend = findBackend();
    QVERIFY2(backend, "SudokuGrid.backend not found");

    // 1. 풀이 중이면 중단 (isBusy면 clear()가 아무 일도 하지 않으므로 필수)
    if (backend->isBusy()) {
        backend->stop();
        QTRY_VERIFY(!backend->isBusy());
    }

    // 시각화 및 지연 시간 기본값 복원 (앞선 테스트가 예외로 실패하더라도 상태 오염 방지)
    if (backend->visualize() != m_defaultVisualize)
        backend->setVisualize(m_defaultVisualize);
    if (backend->delay() != m_defaultDelay)
        backend->setDelay(m_defaultDelay);

    // 2. UI 상태: 포커스 대상 초기화 (리셋 전에 해야 새 델리게이트가 앞 테스트의 포커스를 이어받지 않음)
    auto* grid = m_window->findChild<QQuickItem*>(QStringLiteral("sudokuGrid"));
    QVERIFY2(grid, "sudokuGrid not found");
    grid->setProperty("focusedIndex", -1);

    // 3. repeater 획득 및 모델 리셋 전 첫 번째 델리게이트 추적
    auto* repeater = sudoku_test::findRepeater(m_window);
    QVERIFY(repeater);
    QPointer<QQuickItem> oldFirstCell = sudoku_test::fetchCellFromRepeater(repeater, 0);
    backend->clear();

    // 4. 옛 델리게이트 파괴와 새 델리게이트 생성 완료까지 대기
    if (oldFirstCell)
        QTRY_VERIFY(oldFirstCell.isNull());
    QTRY_COMPARE(repeater->property("count").toInt(), 81);
    QVERIFY(!backend->hasErrors());
}

QTEST_MAIN(TestSudokuGui)
#include "tst_sudoku_gui.moc"
