#ifndef SUDOKU_SOLVER_H
#define SUDOKU_SOLVER_H

#include <QVector>

enum class SolveAlgorithm {
    BackTracking,
    // 추후 추가
};

using StepCallback = std::function<bool(const QVector<QVector<int>>&)>; // bool 리턴은 중단 여부

class SudokuSolver
{
public:
    explicit SudokuSolver();

    // 유효성 검사
    static bool isValid(const QVector<QVector<int>> &board, int r, int c, int num, bool ignoreSelf = false);

    // 백트래킹 풀이
    // callback이 nullptr이면 일반 실행, 있으면 매 단계 호출
    bool solve(QVector<QVector<int>> &board,
               SolveAlgorithm algorithm = SolveAlgorithm::BackTracking,
               StepCallback callback = nullptr);

    // 퍼즐 생성
    void generate(QVector<QVector<int>> &board, int difficulty);

private:
    bool solveBacktracking(QVector<QVector<int>> &board, StepCallback callback  );

    bool solveRandomly(QVector<QVector<int>> &board);
};

#endif // SUDOKU_SOLVER_H
