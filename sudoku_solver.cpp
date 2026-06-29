#include "sudoku_solver.h"
#include <random>

SudokuSolver::SudokuSolver() {}

bool SudokuSolver::isValid(const QVector<QVector<int>> &board, int r, int c, int num, bool ignoreSelf)
{
    // 같은 행/열 체크
    for(int i{0}; i < 9; ++i) {
        // 행 검사: (r, i) 위치 확인
        if(board[r][i] == num) {
            if(!ignoreSelf || i != c) return false; // 내 위치가 아니거나, ignoreSelf가 false일 때만 충돌
        }
        // 열 검사: (i, c) 위치 확인
        if(board[i][c] == num) {
            if(!ignoreSelf || i != r) return false;
        }
    }

    // 3x3 박스 체크
    int startRow = r - r % 3;
    int startCol = c - c % 3;
    for(int i{0}; i < 3; ++i) {
        for(int j{0}; j < 3; ++j) {
            int checkRow = startRow + i;
            int checkCol = startCol + j;
            if(board[checkRow][checkCol] == num)  {
                if(!ignoreSelf || (checkRow != r || checkCol != c)) return false;
            }
        }
    }
    return true;
}

SolveResult SudokuSolver::solve(QVector<QVector<int>> &board, SolveAlgorithm algorithm, StepCallback callback)
{
    switch (algorithm) {
    case SolveAlgorithm::BackTracking:
        return solveBacktracking(board, callback);
    default:
        return solveBacktracking(board, callback);
    }
}

void SudokuSolver::generate(QVector<QVector<int>> &board, int difficulty)
{
    // 1. 보드 초기화
    for(int i{0}; i < 9; ++i)
        board[i].fill(0);

    // 2. 스도쿠 판 무작위 채우기
    solveRandomly(board);

    // 3. 난이도별 타겟 지우기 개수 정의 (Easy: 32, Midium: 42, Hard: 52)
    int targetRemoveCount{std::clamp(0, 32 + difficulty * 10, 64)};
    int currentRemoved{0};

    // 0 ~ 80 인덱스 셔플
    QVector<int> indices(81);
    std::iota(indices.begin(), indices.end(), 0);

    thread_local std::random_device rd;
    thread_local std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);

    // 4. 셀을 하나씩 지우며 유일해 검증
    for(int i{0}; i < 81; ++i) {
        if(currentRemoved >= targetRemoveCount)
            break;

        int idx = indices[i];
        int r{idx / 9};
        int c{idx % 9};

        // 이미 지워진 셀이 아니면 시도
        if (board[r][c] != 0) {
            int backup{board[r][c]};
            board[r][c] = 0;

            // 유일해 검사 수행
            if (hasUniqueSolution(board)) {
                ++currentRemoved; // 유일해가 유지되므로 지운 상태 확정
            } else {
                board[r][c] = backup; // 유일해가 깨지면 원래대로 복원
            }
        }
    }
}

bool SudokuSolver::hasUniqueSolution(QVector<QVector<int>>& board)
{
    return countSolutions(board, 2) == 1;
}

// --- private ---

SolveResult SudokuSolver::solveBacktracking(QVector<QVector<int>> &board, StepCallback callback)
{
    for(int r = 0; r < 9; ++r) {
        for(int c = 0; c < 9; ++c) {
            if(board[r][c] == 0) {
                for(int num{1}; num <= 9; ++num) {
                    if(isValid(board, r, c, num)) {
                        board[r][c] = num;

                        // 시각화 및 중단 훅
                        if(callback) {
                            if(!callback(board)) {
                                board[r][c] = 0; // 중단 즉시 자신이 넣었던 값을 0으로 리셋
                                return SolveResult::Aborted; // 사용자가 중단 요청
                            }
                        }

                        SolveResult result = solveBacktracking(board, callback);
                        if(result == SolveResult::Success)
                            return SolveResult::Success;

                        // 실패했거나 중단되었으므로 대입한 값을 0으로 원상 복구
                        board[r][c] = 0;

                        // 중단 상태라면 다음 루프를 돌지 않고 즉시 상위 스택으로 전파
                        if(result == SolveResult::Aborted)
                            return SolveResult::Aborted;

                        // 시각화 및 중단 훅 (지울 때)
                        if(callback) {
                            if(!callback(board))
                                return SolveResult::Aborted; // 중단 즉시 반환
                        }
                    }
                }
                return SolveResult::Failed;
            }
        }
    }
    return SolveResult::Success;
}

bool SudokuSolver::solveRandomly(QVector<QVector<int>> &board)
{
    for(int r{0}; r < 9; ++r) {
        for(int c{0}; c < 9; ++c) {
            if(board[r][c] == 0) {
                std::array<int, 9> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9};
                thread_local std::random_device rd;
                thread_local std::mt19937 g(rd());
                std::shuffle(nums.begin(), nums.end(), g);

                for(int num : nums) {
                    if(isValid(board, r, c, num)) {
                        board[r][c] = num;
                        if(solveRandomly(board)) return true;
                        board[r][c] = 0;
                    }
                }
                return false;
            }
        }
    }
    return true;
}

int SudokuSolver::countSolutions(QVector<QVector<int>>& board, int maxCount)
{
    if (maxCount <= 0)
        return 0;

    for(int r{0}; r < 9; ++r) {
        for(int c{0}; c < 9; ++c) {
            if(board[r][c] == 0) {
                int count{0};
                for(int num{1}; num <= 9; ++num) {
                    if(isValid(board, r, c, num)) {
                        board[r][c] = num;
                        count += countSolutions(board, maxCount - count);
                        board[r][c] = 0; // backtrack

                        if(count >= maxCount) return count;
                    }
                }
                return count;
            }
        }
    }
    return 1;
}