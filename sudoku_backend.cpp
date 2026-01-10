#include "sudoku_backend.h"

SudokuBackend::SudokuBackend(QObject *parent)
    : QObject{parent}
{
    m_board.resize(9);
    for(int i{0}; i < 9; ++i) {
        m_board[i].resize(9);
        m_board[i].fill(0);
    }
}

// 2차원 데이터를 QML UI 전용 1차원 리스트(Flatten)로 변환
QList<int> SudokuBackend::board() const
{
    QList<int> flatList;
    flatList.reserve(81);
    for(const auto& row : m_board) {
        for(int val : row) {
            flatList.append(val);
        }
    }
    return flatList;
}

void SudokuBackend::setCell(int index, int value)
{
    if(index < 0 || index >= 81) return;
    int r = index / 9;
    int c = index % 9;

    if(value < 0 || value > 9) return;

    if(m_board[r][c] != value) {
        m_board[r][c] = value;
        emit boardChanged();
    }
}

void SudokuBackend::clear()
{
    for(int i{0}; i < 9; ++i) {
        m_board[i].fill(0);
    }
    emit boardChanged();
}

bool SudokuBackend::isValidBoard()
{
    // 모든 채워진 칸에 대해 규칙 위반 여부 검사
    for(int r = 0; r < 9; ++r) {
        for(int c = 0; c < 9; ++c) {
            int num = m_board[r][c];
            if(num != 0) {
                // 현재 위치(r,c)의 값을 잠시 0으로 비움 (자기 자신과 충돌 방지)
                m_board[r][c] = 0;

                // 해당 숫자가 유효한지 검사
                if(!isValid(m_board, r, c, num)) {
                    // 유효하지 않으면 false 리턴
                    m_board[r][c] = num;
                    return false;
                }

                // 원상복구
                m_board[r][c] = num;
            }
        }
    }
    return true;
}

// --- algorithm ---
bool SudokuBackend::solveBacktracking()
{
    QVector<QVector<int>> tempBoard = m_board;
    if(solveBacktrackingHelper(tempBoard)) {
        m_board = tempBoard ;
        emit boardChanged();
        return true;
    }
    return false;
}


//  --- private ---
bool SudokuBackend::isValid(const QVector<QVector<int>> &board, int r, int c, int num)
{
    // 같은 행/열 체크
    for(int i{0}; i < 9; ++i) {
        if(board[r][i] == num) return false;
        if(board[i][c] == num) return false;
    }

    // 3x3 박스 체크
    int startRow = r - r % 3;
    int startCol = c - c % 3;
    for(int i{0}; i < 3; ++i) {
        for(int j{0}; j < 3; ++j) {
            if(board[startRow + i][startCol + j] == num)
                return false;
        }
    }
    return true;
}

bool SudokuBackend::solveBacktrackingHelper(QVector<QVector<int>> &board)
{
    for(int r = 0; r < 9; ++r) {
        for(int c = 0; c < 9; ++c) {
            if(board[r][c] == 0) {
                for(int num{1}; num <= 9; ++num) {
                    if(isValid(board, r, c, num)) {
                        board[r][c] = num;
                        if(solveBacktrackingHelper(board))
                            return true;
                        board[r][c] = 0; // backtrack
                    }
                }
                return false;
            }
        }
    }
    return true;
}
