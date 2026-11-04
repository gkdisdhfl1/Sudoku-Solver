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
    void cleanup();

    // 1. 숫자 클릭, 덮어쓰기 및 지우기 무결성 검증
    void testCellClickAndTypeOverwrite();

    // 2. 방향키 네비게이션 및 경계 조건 검증
    void testArrowKeyNavigation();

    // 3. 모델 리셋(clear) 후에도 이전에 얻은 CellDriver가 새 델리게이트를 가리키는지 검증
    void testCellDriverSurvivesModelReset();

private:
    SudokuBackend* findBackend() const;
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

    // 빌드 디렉토리에 생성된 SudokuSolver QML 모듈 경로 추가
    #ifdef TEST_QML_IMPORT_DIR
    m_engine->addImportPath(TEST_QML_IMPORT_DIR);
    #endif

    m_engine->loadFromModule("SudokuSolver", "Main");

    const auto rootObjects{m_engine->rootObjects()};
    QVERIFY2(!rootObjects.isEmpty(), "Main.qml failed to load!");

    m_window = qobject_cast<QQuickWindow*>(rootObjects.first());
    QVERIFY2(m_window != nullptr, "Root object is not a QQuickWindow!");

    m_window->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
}

void TestSudokuGui::cleanupTestCase()
{
    if (m_window) {
        m_window->close();
    }
    m_engine.reset();
}

void TestSudokuGui::cleanup()
{
    /// 각 테스트가 끝날 때마다 (0, 0) 셀을 0으로 리셋
    if (m_window) {
        SudokuDriver driver(m_window);
        driver.cell(0, 0).click().press(Qt::Key_Delete);
    }
}

void TestSudokuGui::testCellClickAndTypeOverwrite()
{
    SudokuDriver driver(m_window);

    // 1. (0, 0) 셀 클릭 후 숫자 '5' 입력
    driver.cell(0, 0).click().type("5");
    QCOMPARE(driver.cell(0, 0).value(), 5);

    // 2. 이미 숫자가 채워진 상태에서 숫자 '9' 입력 -> 벡스페이스 없이 즉시 '9'로 덮어써져야 함
    driver.cell(0, 0).type("9");
    QCOMPARE(driver.cell(0, 0).value(), 9);

    // 3. Backspace 키 입력 -> 즉시 0(빈 칸)으로 지워져야 함
    driver.cell(0, 0).press(Qt::Key_Backspace);
    QCOMPARE(driver.cell(0, 0).value(), 0);

    // 4. 숫자 '3' 입력 후 '0' 키 입력 -> 지워져야 함
    driver.cell(0, 0).type("3");
    QCOMPARE(driver.cell(0, 0).value(), 3);
    driver.cell(0, 0).type("0");
    QCOMPARE(driver.cell(0, 0).value(), 0);
}

void TestSudokuGui::testArrowKeyNavigation()
{
    SudokuDriver driver(m_window);

    // 1. (0, 0) 셀을 클릭하여 포커스 획득
    driver.cell(0, 0).click();
    QVERIFY(driver.cell(0, 0).isFocused());

    // 2. 오른쪽 방향키 -> (0, 1)로 포커스 이동
    driver.press(Qt::Key_Right);
    QVERIFY(driver.cell(0, 1).isFocused());

    // 3. 아래쪽 방향키 -> (1, 1)로 포커스 이동
    driver.press(Qt::Key_Down);
    QVERIFY(driver.cell(1, 1).isFocused());

    // 4. 왼쪽 방향키 -> (1, 0)로 포커스 이동
    driver.press(Qt::Key_Left);
    QVERIFY(driver.cell(1, 0).isFocused());

    // 5. 위쪽 방향키 -> (0, 0)로 복귀
    driver.press(Qt::Key_Up);
    QVERIFY(driver.cell(0, 0).isFocused());

    // 6. 경계 조건 검증: 최상단 (0, 0)에서 위/왼쪽 키를 눌러도 보드 밖으로 튀지 않고 (0, 0) 유지
    driver.press(Qt::Key_Left);
    QVERIFY(driver.cell(0, 0).isFocused());
    driver.press(Qt::Key_Up);
    QVERIFY(driver.cell(0, 0).isFocused());
}

void TestSudokuGui::testCellDriverSurvivesModelReset()
{
    SudokuBackend* backend = findBackend();
    QVERIFY2(backend, "SudokuGrid.backend not found");

    // 리셋 이전에 핸들을 얻어 둔다 (임시 SudokuDriver에서 받은 값 복사 핸들)
    CellDriver cell{SudokuDriver::on(m_window).cell(0, 0)};
    CellDriver neighbor{cell.cell(1, 1)}; // 체이닝으로 얻은 핸들도 함게 검증

    cell.click().type("5");
    QCOMPARE(cell.value(), 5);

    // 리셋 전 델리게이트를 약한 참조로 보관 (파괴 완료를 동기화 조건으로 사용)
    QPointer<QQuickItem> oldCell = cell.item();
    QPointer<QQuickItem> oldNeighbor = neighbor.item();

    // Repeater가 델리게이트를 모두 파괴/재생성하도록 모델 리셋 유발
    QMetaObject::invokeMethod(backend, &SudokuBackend::clear);

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

QTEST_MAIN(TestSudokuGui)
#include "tst_sudoku_gui.moc"