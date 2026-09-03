#include <QtTest>
#include <qcontainerfwd.h>
#include <qtestcase.h>
#include "sudoku_solver.h"

class TestSudokuSolver : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testIsValid();
    void testSolveBacktracking();
    void testSolveRandomly();
    void testSolveMRV();
    void testSolveUnsolvable();
    void testSolveCallbackAbort();
    void testGenerateAndUniqueSolution();

private:
    QVector<QVector<int>> createSamplePuzzle();
    bool isCompletedBoardValid(const QVector<QVector<int>>& board);
};

void TestSudokuSolver::initTestCase()
{
    qRegisterMetaType<StepInfo>("StepInfo");
}

QVector<QVector<int>> TestSudokuSolver::createSamplePuzzle()
{
    // 표준 난이도의 검증된 샘플 스도쿠 퍼즐
    return {
        {5, 3, 0, 0, 7, 0, 0, 0, 0},
        {6, 0, 0, 1, 9, 5, 0, 0, 0},
        {0, 9, 8, 0, 0, 0, 0, 6, 0},
        {8, 0, 0, 0, 6, 0, 0, 0, 3},
        {4, 0, 0, 8, 0, 3, 0, 0, 1},
        {7, 0, 0, 0, 2, 0, 0, 0, 6},
        {0, 6, 0, 0, 0, 0, 2, 8, 0},
        {0, 0, 0, 4, 1, 9, 0, 0, 5},
        {0, 0, 0, 0, 8, 0, 0, 7, 9}
    };
}

bool TestSudokuSolver::isCompletedBoardValid(const QVector<QVector<int>>& board)
{
    for (int r{0}; r < 9; ++r) {
        for (int c{0}; c < 9; ++c) {
            int num{board[r][c]};
            if (num < 1 || num > 9) return false;
            if (!SudokuSolver::isValid(board, r, c, num, true)) return false;
        }
    }
    return true;
}

void TestSudokuSolver::testIsValid()
{
    auto board = createSamplePuzzle();

    // 1. 빈 칸(0, 2)에 유효한 숫자 1 또는 4 또는 2 등을 놓을 수 있는지 검증
    QVERIFY(SudokuSolver::isValid(board, 0, 2, 1));
    QVERIFY(SudokuSolver::isValid(board, 0, 2, 4));

    // 2. 같은 행(Row 0)에 이미 있는 5, 3, 7은 놓을 수 없어야 함
    QVERIFY(!SudokuSolver::isValid(board, 0, 2, 5));
    QVERIFY(!SudokuSolver::isValid(board, 0, 2, 3));
    QVERIFY(!SudokuSolver::isValid(board, 0, 2, 7));

    // 3. 같은 열(Col 2)에 이미 있는 8은 놓을 수 없어야 함
    QVERIFY(!SudokuSolver::isValid(board, 0, 2, 8));

    // 4. 같은 3x3 박스 내에 이미 있는 6, 9는 놓을 수 없어야 함
    QVERIFY(!SudokuSolver::isValid(board, 0, 2, 6));
    QVERIFY(!SudokuSolver::isValid(board, 0, 2, 9));
}

void TestSudokuSolver::testSolveBacktracking()
{
    SudokuSolver solver;
    auto board = createSamplePuzzle();

    SolveResult result = solver.solve(board, SudokuSolver::SolveAlgorithm::BackTracking);
    QCOMPARE(result, SolveResult::Success);
    QVERIFY(isCompletedBoardValid(board));
}

void TestSudokuSolver::testSolveRandomly()
{
    SudokuSolver solver;
    auto board = createSamplePuzzle();

    SolveResult result = solver.solve(board, SudokuSolver::SolveAlgorithm::Randomly);
    QCOMPARE(result, SolveResult::Success);
    QVERIFY(isCompletedBoardValid(board));
}

void TestSudokuSolver::testSolveMRV()
{
    SudokuSolver solver;
    auto board = createSamplePuzzle();

    SolveResult result = solver.solve(board, SudokuSolver::SolveAlgorithm::MRV);
    QCOMPARE(result, SolveResult::Success);
    QVERIFY(isCompletedBoardValid(board));
}

void TestSudokuSolver::testSolveUnsolvable()
{
    SudokuSolver solver;

    // 1. 초기 상태에 중복이 있는 명백히 모순된 퍼즐 -> 즉시 Failed 반환 검증
    QVector<QVector<int>> duplicateBoard(9, QVector<int>(9, 0));
    duplicateBoard[0][0] = 5;
    duplicateBoard[0][1] = 5;
    QCOMPARE(solver.solve(duplicateBoard, SudokuSolver::SolveAlgorithm::BackTracking), SolveResult::Failed);

    // 2. 초기 중복은 없으나 논리적으로 해가 존재하지 않는 퍼즐 -> 탐색 후 Failed 반환 검증
    QVector<QVector<int>> noSolutionBoard = {
        {5, 1, 6, 8, 4, 9, 7, 3, 2},
        {3, 0, 7, 6, 0, 5, 0, 0, 0},
        {8, 0, 9, 7, 0, 0, 0, 6, 5},
        {1, 3, 5, 0, 6, 0, 9, 0, 7},
        {4, 7, 2, 5, 9, 1, 0, 0, 6},
        {9, 6, 8, 3, 7, 0, 0, 5, 0},
        {2, 5, 3, 1, 8, 6, 0, 7, 4},
        {6, 8, 4, 2, 0, 7, 5, 0, 0},
        {7, 9, 1, 0, 5, 0, 6, 0, 8}
    };
    QCOMPARE(solver.solve(noSolutionBoard, SudokuSolver::SolveAlgorithm::BackTracking), SolveResult::Failed);
}

void TestSudokuSolver::testSolveCallbackAbort()
{
    SudokuSolver solver;
    auto board = createSamplePuzzle();

    int stepCount{0};
    // 5번째 스텝에서 탐색 중단(false 반환) 요청
    auto abortCallback = [&stepCount](const StepInfo&) -> bool {
        stepCount++;
        return stepCount < 5; // 5번재 스텝에서 false 반환
    };

    SolveResult result = solver.solve(board, SudokuSolver::SolveAlgorithm::BackTracking, abortCallback);
    QCOMPARE(result, SolveResult::Aborted);
    QCOMPARE(stepCount, 5);
}

void TestSudokuSolver::testGenerateAndUniqueSolution()
{
    SudokuSolver solver;

    for (int diff{0}; diff < 3; ++diff) {
        QVector<QVector<int>> board(9, QVector<int>(9, 0));
        bool generated = solver.generate(board, diff);
        QVERIFY(generated);

        // 생성된 퍼즐이 반드시 유일해를 갖는지 검증
        QVERIFY(solver.hasUniqueSolution(board));
    }
}

QTEST_MAIN(TestSudokuSolver)
#include "tst_sudoku_solver.moc"