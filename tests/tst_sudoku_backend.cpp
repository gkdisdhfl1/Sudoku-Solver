#include <QtTest>
#include <QSignalSpy>
#include <qtest.h>
#include <qtestcase.h>
#include "sudoku_backend.h"

class TestSudokuBackend : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialState();
    void testSetCellAndPeerCandidateCache();
    void testErrorDetectionAndSignals();
    void testClearBoard();
};

void TestSudokuBackend::initTestCase()
{
    qRegisterMetaType<StepInfo>("StepInfo");
}

void TestSudokuBackend::testInitialState()
{
    SudokuBackend backend;

    QCOMPARE(backend.rowCount(), 81);
    QCOMPARE(backend.hasErrors(), false);
    QCOMPARE(backend.isBusy(), false);

    // 초기 빈 보드는 모든 셀이 0이어야 함
    for (int i{0}; i < 81; ++i) {
        QModelIndex idx = backend.index(i, 0);
        QCOMPARE(backend.data(idx, SudokuBackend::ValueRole).toInt(), 0);
        QCOMPARE(backend.data(idx, SudokuBackend::ErrorRole).toBool(), false);

        // 빈 보드의 후보는 1~9 전체여야 함
        QVariantList cands = backend.data(idx, SudokuBackend::CandidatesRole).toList();
        QCOMPARE(cands.size(), 9);
    }
}

void TestSudokuBackend::testSetCellAndPeerCandidateCache()
{
    SudokuBackend backend;

    // (0,0) 즉 index 0번에 숫자 5를 입력
    backend.setCell(0, 5);
    QCOMPARE(backend.data(backend.index(0, 0), SudokuBackend::ValueRole).toInt(), 5);

    // 같은 행의 (0, 1) 즉 index 1번 셀의 후보에서 5가 제거되었는지 검증
    QVariantList peerCands = backend.data(backend.index(1, 0), SudokuBackend::CandidatesRole).toList();
    QVERIFY(!peerCands.contains(5));
    QCOMPARE(peerCands.size(), 8);

    // 같은 3x3 박스의 (1, 1) 즉 index 10번 셀의 후보에서도 5가 제거되었는지 검증
    QVariantList boxPeerCands = backend.data(backend.index(10, 0), SudokuBackend::CandidatesRole).toList();
    QVERIFY(!boxPeerCands.contains(5));
}

void TestSudokuBackend::testErrorDetectionAndSignals()
{
    SudokuBackend backend;
    QSignalSpy errorSignalSpy(&backend, &SudokuBackend::hasErrorsChanged);

    // 1. (0, 0)에 5를 넣음 -> 정상
    backend.setCell(0, 5);
    QCOMPARE(backend.hasErrors(), false);
    QCOMPARE(errorSignalSpy.count(), 0);

    // 2. 같은 행인 (0, 3)에도 5를 넣음 -> 중복 에러 발생
    backend.setCell(3, 5);
    QCOMPARE(backend.hasErrors(), true);
    QCOMPARE(errorSignalSpy.count(), 1); // hasErrorsChanged 시그널 방출 확인

    // 두 셀 모두 ErrorRole이 true인지 검증
    QCOMPARE(backend.data(backend.index(0, 0), SudokuBackend::ErrorRole).toBool(), true);
    QCOMPARE(backend.data(backend.index(3, 0), SudokuBackend::ErrorRole).toBool(), true);

    // 3. 중복된 (0, 3)을 0으로 지우면 -> 에러 해소
    backend.setCell(3, 0);
    QCOMPARE(backend.hasErrors(), false);
    QCOMPARE(errorSignalSpy.count(), 2);
    QCOMPARE(backend.data(backend.index(0, 0), SudokuBackend::ErrorRole).toBool(), false);
}

void TestSudokuBackend::testClearBoard()
{
    SudokuBackend backend;
    backend.setCell(0, 5);
    backend.setCell(1, 5); // 에러 상태 만듦
    QCOMPARE(backend.hasErrors(), true);

    backend.clear();

    QCOMPARE(backend.hasErrors(), false);
    for (int i{0}; i < 81; ++i) {
        QCOMPARE(backend.data(backend.index(i, 0), SudokuBackend::ValueRole).toInt(), 0);
    }
}

QTEST_GUILESS_MAIN(TestSudokuBackend)
#include "tst_sudoku_backend.moc"