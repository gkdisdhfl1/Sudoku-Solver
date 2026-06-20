#include "solver_worker.h"
#include "sudoku_solver.h"

#include <QThread>

SolverWorker::SolverWorker(const QVector<QVector<int>> &board,
                           JobType jobType,
                           bool visualize,
                           int delay,
                           int difficulty,
                           QObject *parent)
    : QObject{parent}
    , m_board(board)
    , m_jobType(jobType)
    , m_visualize(visualize)
    , m_delay(delay)
    , m_difficulty(difficulty)
{}

void SolverWorker::requestStop()
{
    QMutexLocker locker(&m_pauseMutex);
    m_stopRequested = true;

    // 잠자고 있을 수도 있는 스레드를 강제로 깨워 중단 요청을 확인하게 함
    m_pauseCondition.wakeAll();
}

void SolverWorker::process()
{
    SudokuSolver solver;
    SolveResult result;

    // 역할에 따른 두 개의 콜백 변수 정의
    auto stopOnlyCallback = [this](const QVector<QVector<int>>&) -> bool {
        return !m_stopRequested;
    };

    auto visualizeCallback = [this](const QVector<QVector<int>>& board) -> bool {
        return processStep(board);
    };

    if(m_jobType == JobType::Solve) {
        if(m_visualize) {
            m_updateTimer.start(); // 타이머 시작

            // 시각화 전용 콜백 주입
            result = solver.solve(m_board, SolveAlgorithm::BackTracking, visualizeCallback);

            // 루프가 끝난 후 마지막 상태는 반드시 업데이트 (누락 방지)
            emit boardUpdated(m_board);
        } else {
            // 비시각화 중단용 콜백 주입
            result = solver.solve(m_board, SolveAlgorithm::BackTracking, stopOnlyCallback);
        }

        // 성공 여부 판단 및 최종 보드 업데이트
        bool success = (result == SolveResult::Success);
        if (success) {
            emit boardUpdated(m_board);
        }
        emit finished(success);

    } else if(m_jobType == JobType::Generate) {
        solver.generate(m_board, m_difficulty);
        emit boardUpdated(m_board);
        emit finished(true);
    }
}

void SolverWorker::setPaused(bool paused)
{
    QMutexLocker locker(&m_pauseMutex);

    // 이미 같은 상태면 무시
    if(m_isPaused == paused) return;

    m_isPaused = paused;

    if(!paused) {
        // 재개할 때는 잠자던 스레드를 깨워줌
        m_pauseCondition.wakeAll();
    }
}

bool SolverWorker::processStep(const QVector<QVector<int>>& currentBoard)
{
    if(m_stopRequested) return false;

    // 1. 일시 정지 (Pause) 처리
    if(m_isPaused) {
        QMutexLocker locker(&m_pauseMutex);
        while (m_isPaused) {
            if(m_stopRequested) return false;
            m_pauseCondition.wait(&m_pauseMutex);
        }
    }

    // 2. UI 갱싱 (Throttling)
    if(m_delay > 10 || m_updateTimer.elapsed() >= 16) {
        emit boardUpdated(currentBoard);
        m_updateTimer.restart();
    }

    // 3. 지연 (Smart Sleep)
    if(m_delay > 0) {
        QMutexLocker locker(&m_pauseMutex);
        // m_delay 만큼 대기하되 requestStop()이 wakeAll()을 부르면 즉시 깨어남
        if(!m_stopRequested) { // 이미 stop 요청이 왔다면 잘 필요 없음
            m_pauseCondition.wait(&m_pauseMutex, m_delay);
        }
    }

    return true; // 계속 진행
}
