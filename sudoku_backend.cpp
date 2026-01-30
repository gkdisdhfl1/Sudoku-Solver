#include "sudoku_backend.h"

#include "sudoku_solver.h"

#include <QCoreApplication>
#include <QThread>

SudokuBackend::SudokuBackend(QObject *parent)
    : QObject{parent}
{
    m_board.resize(9);
    for(int i{0}; i < 9; ++i) {
        m_board[i].resize(9);
        m_board[i].fill(0);
    }
}

SudokuBackend::~SudokuBackend()
{
    stop(); // 종료 시 스레드 정리
    if(m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
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

QList<int> SudokuBackend::errorCells() const
{
    return m_errorCells;
}

bool SudokuBackend::visualize() const
{
    return m_visualize;
}

void SudokuBackend::setVisualize(bool v)
{
    if(m_visualize != v) {
        m_visualize = v;
        emit visualizeChanged();
    }
}

int SudokuBackend::delay() const
{
    return m_delay;
}

void SudokuBackend::setDelay(int d)
{
    if(m_delay != d)
    {
        m_delay = d;
        emit delayChanged();
    }
}

bool SudokuBackend::isBusy() const
{
    return m_isBusy;
}

bool SudokuBackend::isPaused() const
{
    return m_isPaused;
}

void SudokuBackend::setCell(int index, int value)
{
    if(m_isBusy) return; // 작업 중엔 수정 불가

    if(index < 0 || index >= 81) return;
    int r = index / 9;
    int c = index % 9;

    if(value < 0 || value > 9) return;

    if(m_board[r][c] != value) {
        m_board[r][c] = value;
        emit boardChanged();

        // 값이 바뀔 때마다 에러 상태 갱신
        checkErrors();
    }
}

void SudokuBackend::clear()
{
    if(m_isBusy) return; // 작업 중엔 수정 불가

    for(int i{0}; i < 9; ++i) {
        m_board[i].fill(0);
    }
    emit boardChanged();

    // 클리어 시 에러 초기화
    checkErrors();
}

bool SudokuBackend::isValidBoard() const
{
    // 모든 채워진 칸에 대해 규칙 위반 여부 검사
    for(int r = 0; r < 9; ++r) {
        for(int c = 0; c < 9; ++c) {
            int num = m_board[r][c];
            if(num != 0) {
                // 해당 숫자가 유효한지 검사
                if(!SudokuSolver::isValid(m_board, r, c, num, true)) {
                    // 유효하지 않으면 false 리턴
                    return false;
                }
            }
        }
    }
    return true;
}

// --- 메인 기능 ---
void SudokuBackend::solveBacktracking()
{
    if(m_isBusy) return;

    m_isBusy = true;
    emit isBusyChanged();

    m_workerThread = new QThread;
    // Worker 생성 (Solve 모드)
    m_worker = new SolverWorker(m_board, SolverWorker::JobType::Solve, m_visualize, m_delay);
    m_worker->moveToThread(m_workerThread);

    // 시그널 연결
    connect(m_workerThread, &QThread::started, m_worker, &SolverWorker::process);
    connect(m_worker, &SolverWorker::boardUpdated, this, &SudokuBackend::handleBoardUpdate);
    connect(m_worker, &SolverWorker::finished, this, &SudokuBackend::handleWorkerFinished);

    // 자동 정리 연결
    connect(m_worker, &SolverWorker::finished, m_workerThread, &QThread::quit);
    connect(m_worker, &SolverWorker::finished, m_worker, &SolverWorker::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater);

    // 스레드 종료 시 멤버 포인터 초기화는 handleWorkerFinished에서 수행하는 게 안전
    m_workerThread->start();
}

void SudokuBackend::generatePuzzle(int difficulty)
{
    if(m_isBusy) return;

    m_isBusy = true;
    emit isBusyChanged();

    m_workerThread = new QThread;
    // Worker 생성 (Generate 모드)
    m_worker = new SolverWorker(m_board, SolverWorker::JobType::Generate, false, 0, difficulty); // 생성은 시각화 없이 즉시 수행
    m_worker->moveToThread(m_workerThread);

    // 시그널 연결
    connect(m_workerThread, &QThread::started, m_worker, &SolverWorker::process);
    connect(m_worker, &SolverWorker::boardUpdated, this, &SudokuBackend::handleBoardUpdate);
    connect(m_worker, &SolverWorker::finished, this, &SudokuBackend::handleWorkerFinished);
    connect(m_worker, &SolverWorker::finished, m_worker, &SolverWorker::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater);

    m_workerThread->start();
}

void SudokuBackend::stop()
{
    if(m_worker && m_isBusy) {
        m_worker->requestStop();
        // 스레드가 종료될 때까지 기다리지 않음
        // 종료되면 handleWorkerFinished가 호출됨.
    }
}

void SudokuBackend::togglePause()
{
    if(!m_worker || !m_isBusy) return;

    m_isPaused = !m_isPaused; // 상태 반전

    // 워커의 멤버 함수를 직접 호출 (atomic 변수 조작이므로 안전)
    m_worker->setPaused(m_isPaused);

    emit isPausedChanged();
}

// -- handler ---
void SudokuBackend::handleBoardUpdate(const QVector<QVector<int>> &board)
{
    m_board = board;
    emit boardChanged();
    // checkErrors(); // 시각화 중 실시간 에러 검사
}

void SudokuBackend::handleWorkerFinished(bool success)
{
    m_isBusy = false;
    m_isPaused = false;
    emit isBusyChanged();
    emit isPausedChanged();

    m_worker = nullptr;
    m_workerThread = nullptr; // deleteLater로 삭제되므로 포인터만 null 처리

    checkErrors(); // 최종 상태 에러 검사

    emit solveFinished(success);
}

//  --- private ---
void SudokuBackend::checkErrors()
{
    QList<int> newErrors;
    for(int r = 0; r < 9; ++r) {
        for(int c = 0; c < 9; ++c) {
            int num = m_board[r][c];
            if(num != 0) {
                if(!SudokuSolver::isValid(m_board, r, c, num, true)) {
                    newErrors.append(r * 9 + c);
                }
            }
        }
    }

    if(m_errorCells != newErrors) {
        m_errorCells = newErrors;
        emit errorCellsChanged();
    }
}
