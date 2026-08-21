#include "sudoku_solver.h"
#include "sudoku_constants.h"

using namespace SudokuConstants;

SudokuSolver::SudokuSolver() {}

bool SudokuSolver::isValid(const QVector<QVector<int>> &board, int r, int c, int num, bool ignoreSelf)
{
    // 같은 행/열 체크
    for (int i{0}; i < 9; ++i) {
        // 행 검사: (r, i) 위치 확인
        if (board[r][i] == num) {
            if (!ignoreSelf || i != c)
                return false; // 내 위치가 아니거나, ignoreSelf가 false일 때만 충돌
        }
        // 열 검사: (i, c) 위치 확인
        if (board[i][c] == num) {
            if (!ignoreSelf || i != r)
                return false;
        }
    }

    // 3x3 박스 체크
    int startRow = r - r % 3;
    int startCol = c - c % 3;
    for (int i{0}; i < 3; ++i) {
        for (int j{0}; j < 3; ++j) {
            int checkRow = startRow + i;
            int checkCol = startCol + j;
            if (board[checkRow][checkCol] == num) {
                if (!ignoreSelf || (checkRow != r || checkCol != c))
                    return false;
            }
        }
    }
    return true;
}

SolveResult SudokuSolver::solve(QVector<QVector<int>> &board, SolveAlgorithm algorithm, StepCallback callback)
{
    switch (algorithm) {
        case SolveAlgorithm::BackTracking:
            return solveBacktracking(board, callback, 0);

        case SolveAlgorithm::Randomly:
            return solveRandomly(board, callback, 0);
        case SolveAlgorithm::MRV:
            return solveMRV(board, callback);        
    }
    Q_UNREACHABLE();
}

bool SudokuSolver::generate(QVector<QVector<int>> &board, int difficulty, StepCallback callback)
{
    // 1. 보드 초기화
    for (int i{0}; i < 9; ++i)
        board[i].fill(0);

    // 2. 스도쿠 판 무작위 채우기
    SolveResult result = solveRandomly(board, callback);
    if (result != SolveResult::Success) {
        return false;
    }

    // 3. 난이도별 타겟 지우기 개수 정의 (Easy: 32, Medium: 42, Hard: 52)
    int targetRemoveCount{std::clamp(BASE_REMOVE_COUNT + difficulty * DIFFICULTY_STEP, MIN_REMOVE_COUNT, MAX_REMOVE_COUNT)};
    int currentRemoved{0};

    // 0 ~ 80 인덱스 셔플
    QVector<int> indices(81);
    std::iota(indices.begin(), indices.end(), 0);

    std::shuffle(indices.begin(), indices.end(), get_thread_local_generator());

    // 4. 셀을 하나씩 지우며 유일해 검증
    for (int idx : indices) {
        // 루프 진입 시마다 중단 요청 여부 체크
        // 없으면 유일해 검증 중 중단이 불가능
        if (callback && !callback({board})) {
            return false;
        }

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

            if (currentRemoved >= targetRemoveCount) {
                break;
            }
        }
    }
    return true;
}

bool SudokuSolver::hasUniqueSolution(QVector<QVector<int>> &board)
{
    return countSolutions(board, UNIQUE_SOLUTION_LIMIT, 0) == 1;
}

// --- private ---

SolveResult SudokuSolver::solveBacktracking(QVector<QVector<int>> &board, StepCallback callback, int idx)
{
    if (idx >= 81)
        return SolveResult::Success;

    int r{idx / 9};
    int c{idx % 9};

    if (board[r][c] == 0) {
        for (int num{1}; num <= 9; ++num) {
            if (isValid(board, r, c, num)) {
                board[r][c] = num;

                // 시각화 및 중단 훅
                if (callback) {
                    if (!callback({board})) {
                        board[r][c] = 0;             // 중단 즉시 자신이 넣었던 값을 0으로 리셋
                        return SolveResult::Aborted; // 사용자가 중단 요청
                    }
                }

                SolveResult result = solveBacktracking(board, callback, idx + 1);
                if (result == SolveResult::Success)
                    return SolveResult::Success;

                // 실패했거나 중단되었으므로 대입한 값을 0으로 원상 복구
                board[r][c] = 0;

                // 중단 상태라면 다음 루프를 돌지 않고 즉시 상위 스택으로 전파
                if (result == SolveResult::Aborted)
                    return SolveResult::Aborted;

                // 시각화 및 중단 훅 (지울 때)
                if (callback) {
                    if (!callback({board}))
                        return SolveResult::Aborted; // 중단 즉시 반환
                }
            }
        }
        return SolveResult::Failed;
    } else {
        // 채워진 칸은 다음 인덱스로 즉시 건너뜀
        return solveBacktracking(board, callback, idx + 1);
    }
}

SolveResult SudokuSolver::solveRandomly(QVector<QVector<int>> &board, StepCallback callback, int idx)
{
    if (idx >= 81)
        return SolveResult::Success;

    int r{idx / 9};
    int c{idx % 9};

    if (board[r][c] == 0) {
        std::array<int, 9> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        std::shuffle(nums.begin(), nums.end(), get_thread_local_generator());

        for (int num : nums) {
            if (isValid(board, r, c, num)) {
                board[r][c] = num;

                if (callback && !callback({board})) {
                    board[r][c] = 0;
                    return SolveResult::Aborted;
                }

                SolveResult result = solveRandomly(board, callback, idx + 1);
                if (result == SolveResult::Success)
                    return SolveResult::Success;

                board[r][c] = 0; // backtrack

                if (result == SolveResult::Aborted)
                    return SolveResult::Aborted;

                if (callback && !callback({board}))
                    return SolveResult::Aborted;
            }
        }
        return SolveResult::Failed;
    } else {
        return solveRandomly(board, callback, idx + 1);
    }
}

SolveResult SudokuSolver::solveMRV(QVector<QVector<int>> &board, StepCallback callback)
{
    int bestR{-1};
    int bestC{-1};
    int minOptions{10}; // 1~9보다 큰 초기값
    QVector<int> bestValidNums;

    // 1. 전체 보드를 스캔하여 잔여 유효 숫자가 가장 적은(MRV) 빈 셀 탐색
    for (int r{0}; r < 9; ++r) {
        for (int c{0}; c < 9; ++c) {
            if (board[r][c] == 0) {
                QVector<int> validNums;
                for (int num{1}; num <= 9; ++num) {
                    if (isValid(board, r, c, num)) {
                        validNums.append(num);
                    }
                }

                // 막다른 골목(들어갈 수 있는 숫자가 0개) 발견 시 즉시 백트랙
                if (validNums.isEmpty()) {
                    return SolveResult::Failed;
                }

                // 잔여 가능값 최저 셀 갱신
                if (validNums.size() < minOptions) {
                    minOptions = validNums.size();
                    bestR = r;
                    bestC = c;
                    bestValidNums = validNums;

                    // 1개만 가능한 제약 최상위 셀을 찾았다면 탐색 중단하고 즉시 선택 (최적화)
                    if (minOptions == 1) break;
                }
            }
        }
        if (minOptions == 1) break;
    }

    // 더 이상 빈 셀이 없으면 완성
    if (bestR == -1) {
        return SolveResult::Success;
    }

    // 후보 숫자가 정해지면 외부 콜백으로 전파
    if (callback) {
        if (!callback({board, bestR, bestC, bestValidNums})) {
            return SolveResult::Aborted;
        }
    }

    // 2. 선택된 MRV 셀(bestR, bestC)에 유효한 숫자들을 대입해보며 백트래킹
    for (int num : bestValidNums) {
        board[bestR][bestC] = num;

        if (callback && !callback({board, bestR, bestC, bestValidNums})) {
            board[bestR][bestC] = 0;
            return SolveResult::Aborted;            
        }

        SolveResult result = solveMRV(board, callback);
        if (result == SolveResult::Success)
            return SolveResult::Success;

        board[bestR][bestC] = 0; // backtrack

        if (result == SolveResult::Aborted)
            return SolveResult::Aborted;

        if (callback && !callback({board, bestR, bestC, bestValidNums}))
            return SolveResult::Aborted;
    }

    return SolveResult::Failed;
}

int SudokuSolver::countSolutions(QVector<QVector<int>> &board, int maxCount, int idx)
{
    if (idx >= 81)
        return 1;
    if (maxCount <= 0)
        return 0;

    int r{idx / 9};
    int c{idx % 9};

    if (board[r][c] == 0) {
        int count{0};
        for (int num{1}; num <= 9; ++num) {
            if (isValid(board, r, c, num)) {
                board[r][c] = num;
                count += countSolutions(board, maxCount - count, idx + 1);
                board[r][c] = 0; // backtrack

                if (count >= maxCount)
                    return count;
            }
        }
        return count;
    } else {
        return countSolutions(board, maxCount, idx + 1);
    }
}

std::mt19937& SudokuSolver::get_thread_local_generator() {
    thread_local std::mt19937 gen(std::random_device{}());
    return gen;
}
