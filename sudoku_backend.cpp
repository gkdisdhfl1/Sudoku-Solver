#include "sudoku_backend.h"

#include "sudoku_solver.h"

#include <QCoreApplication>
#include <QThread>

SudokuBackend::SudokuBackend(QObject *parent)
    : QAbstractListModel{parent}
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

int SudokuBackend::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    
    return 81; // 9x9 스도쿠 판 크기
}

QVariant SudokuBackend::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    
    int idx = index.row();
    if (idx < 0 || idx >= 81)
        return QVariant();

    int r = idx / 9;
    int c = idx % 9;

    if (role == Qt::DisplayRole || role == ValueRole) {
        return m_board[r][c];
    }

    return QVariant();
}

QHash<int, QByteArray> SudokuBackend::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ValueRole] = "value";
    return roles;
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

        // 표준 index() API로 인덱스를 생성하고, 역할 필터 없이 확실하게 dataChanged 발행
        QModelIndex modelIdx = this->index(index, 0);
        emit dataChanged(modelIdx, modelIdx);

        // 값이 바뀔 때마다 에러 상태 갱신
        checkErrors();
    }
}

void SudokuBackend::clear()
{
    if(m_isBusy) return; // 작업 중엔 수정 불가

    // 전체 보드가 지워질 때는 데이터 갱신이 아닌 모델 리셋을 수행하여 뷰를 동기화
    beginResetModel();
    for(int i{0}; i < 9; ++i) {
        m_board[i].fill(0);
    }
    endResetModel();

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
void SudokuBackend::startWorker(SolverWorker::JobType jobType, int difficulty)
{
    m_isBusy = true;
    emit isBusyChanged();

    m_workerThread = new QThread;

    // JobType에 따라 인자가 다르지만, SolverWorker 생성자 활용
    bool visualize = (jobType == SolverWorker::JobType::Solve) ? m_visualize : false;
    int delay = (jobType == SolverWorker::JobType::Solve) ? m_delay : 0;

    // Worker 생성 (Solve 모드)
    m_worker = new SolverWorker(m_board, jobType, visualize, delay, difficulty);
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

void SudokuBackend::solve()
{
    if(m_isBusy) return;
    if(!isValidBoard()) {
        emit solveFinished(false);
        return;
    }

    startWorker(SolverWorker::JobType::Solve);
}

void SudokuBackend::generatePuzzle(int difficulty)
{
    if(m_isBusy) return;

    startWorker(SolverWorker::JobType::Generate, difficulty);
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
    // 비동기 탐색이나 퍼즐 생성에 의해 보드가 전체 갱신될 때도 모델 리셋을 수행
    beginResetModel();
    m_board = board;
    endResetModel();
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
