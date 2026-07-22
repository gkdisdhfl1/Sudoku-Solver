#ifndef SUDOKU_SOLVER_H
#define SUDOKU_SOLVER_H

#include <QVector>
#include <functional>

enum class SolveAlgorithm {
    BackTracking,
    // 추후 추가
};

// 성공 실패, 사용자에 의한 중단을 구분하기 위한 리턴 타입 정의
enum class SolveResult {
    Success,
    Failed,
    Aborted
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
    SolveResult solve(QVector<QVector<int>> &board,
               SolveAlgorithm algorithm = SolveAlgorithm::BackTracking,
               StepCallback callback = nullptr);

    // 퍼즐 생성
    bool generate(QVector<QVector<int>> &board, int difficulty, StepCallback callback = nullptr);

    // 유일해 검증 함수
    bool hasUniqueSolution(QVector<QVector<int>>& board);

private:
    SolveResult solveBacktracking(QVector<QVector<int>> &board, StepCallback callback, int idx = 0);

    SolveResult solveRandomly(QVector<QVector<int>> &board, StepCallback callback, int idx = 0);

    // 해의 개수를 세기 위한 재귀 백트래킹 함수 (최대 2개까지만 카운트)
    int countSolutions(QVector<QVector<int>>& board, int maxCount, int idx = 0);
};

#endif // SUDOKU_SOLVER_H
