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
    m_stopRequested = true;
}

void SolverWorker::process()
{
    SudokuSolver solver;
    bool success{false};

    if(m_jobType == JobType::Solve) {
        if(m_visualize) {
            m_updateTimer.start(); // 타이머 시작

            // 람다 함수로 시각화 로직 주입
            success = solver.solve(m_board, SolveAlgorithm::BackTracking,
                                   [this](const QVector<QVector<int>>& currentBoard) -> bool {
                                       if(m_stopRequested) return false; // 중단 요청

                                       // --- 일시 정지 로직 ---
                                       if(m_isPaused) {
                                           QMutexLocker locker(&m_pauseMutex); // 뮤텍스 잠금
                                           // paused 상태가 풀릴 때까지 대기 (여기서 스레드 멈춤)
                                           while(m_isPaused) {
                                               // 스레드 종료 요청이 오면 대기 풀고 나가야 함
                                               if(m_stopRequested) return false;
                                               m_pauseCondition.wait(&m_pauseMutex);
                                           }
                                       }
                                       // --------------------

                                       // Throttling: 딜레이 짧을 경우 16ms 간격 제한
                                       if(m_delay > 10 || m_updateTimer.elapsed() >= 16) {
                                           emit boardUpdated(currentBoard);
                                           m_updateTimer.restart();
                                       }

                                       if(m_delay > 0)
                                           QThread::msleep(m_delay);

                                        return true; // 계속 진행
                                    }
                        );
            // 루프가 끝난 후 마지막 상태는 반드시 업데이트 (누락 방지)
            emit boardUpdated(m_board);
        } else {
            // 콜백 없이 실행
            success = solver.solve(m_board);
            if(success)
                emit boardUpdated(m_board);
        }
    } else if(m_jobType == JobType::Generate) {
        solver.generate(m_board, m_difficulty);
        emit boardUpdated(m_board);
        success = true;
    }

    emit finished(success);
}

void SolverWorker::setPaused(bool paused)
{
    // 이미 같은 상태면 무시
    if(m_isPaused == paused) return;

    m_isPaused = paused;

    if(!paused) {
        // 재개할 때는 잠자던 스레드를 깨워줌
        m_pauseCondition.wakeAll();
    }
}
