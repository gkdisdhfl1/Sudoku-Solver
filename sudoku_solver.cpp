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

bool SudokuSolver::solve(QVector<QVector<int>> &board, SolveAlgorithm algorithm, StepCallback callback)
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
    // 초기화
    for(int i{0}; i < 9; ++i)
        board[i].fill(0);

    // 무작위 채우기
    solveRandomly(board);

    // 3. 구멍 뚫기
    int removeCount = 35 + (difficulty * 10);
    QVector<int> indices(81);
    std::iota(indices.begin(), indices.end(), 0);

    thread_local std::random_device rd;
    thread_local std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);

    for(int i{0}; i < removeCount; ++i) {
        int idx = indices[i];
        board[idx / 9][idx % 9] = 0;
    }
}

// --- private ---

bool SudokuSolver::solveBacktracking(QVector<QVector<int>> &board, StepCallback callback)
{
    for(int r = 0; r < 9; ++r) {
        for(int c = 0; c < 9; ++c) {
            if(board[r][c] == 0) {
                for(int num{1}; num <= 9; ++num) {
                    // 중단 요청 체크 (콜백이 있으면 실행 전 확인 가능
                    if(isValid(board, r, c, num)) {
                        board[r][c] = num;

                        // 시각화 및 중단 훅
                        if(callback) {
                            if(!callback(board))
                                return false; // 사용자가 중단 요청
                        }

                        if(solveBacktracking(board, callback))
                            return true;

                        board[r][c] = 0; // backtrack

                        // 시각화 및 중단 훅 (지울 때)
                        if(callback) {
                            if(!callback(board)) return false;
                        }
                    }
                }
                return false;
            }
        }
    }
    return true;
}

bool SudokuSolver::solveRandomly(QVector<QVector<int>> &board)
{
    for(int r{0}; r < 9; ++r) {
        for(int c{0}; c < 9; ++c) {
            if(board[r][c] == 0) {
                QVector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9};
                static std::random_device rd;
                static std::mt19937 g(rd());
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
