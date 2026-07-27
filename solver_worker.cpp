#include "solver_worker.h"
#include "sudoku_solver.h"
#include "sudoku_constants.h"

using namespace SudokuConstants;

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
    QMutexLocker locker(&m_pauseMutex); // wakeAll() 때문에 Mutex 필요
    m_stopRequested = true;

    // 잠자고 있을 수도 있는 스레드를 강제로 깨워 중단 요청을 확인하게 함
    m_pauseCondition.wakeAll();
}

void SolverWorker::process()
{
    SudokuSolver solver;
    SolveResult result;

    // 역할에 따른 콜백 변수 정의
    auto stopOnlyCallback = [this](const QVector<QVector<int>>&) -> bool {
        return !m_stopRequested;
    };

    auto visualizeCallback = [this](const QVector<QVector<int>>& board) -> bool {
        return processStep(board);
    };

    if(m_jobType == JobType::Solve) {
        m_accumulatedOverheadTime = std::chrono::nanoseconds(0);

        // 1. 전체 풀이에  걸린 총 벽시계 시간 측정 시작
        auto totalStart = std::chrono::steady_clock::now();

        if(m_visualize) {
            m_updateTimer.start(); // 타이머 시작
            
            result = solver.solve(m_board, SolveAlgorithm::BackTracking, visualizeCallback);
            emit boardUpdated(m_board);
        } else {
            result = solver.solve(m_board, SolveAlgorithm::BackTracking, stopOnlyCallback);
        }

        auto totalEnd = std::chrono::steady_clock::now();

        // 2. 순수 알고리즘 시간 = 전체 벽시계 시간 - 누적 대기 시간
        auto totalWallClock = std::chrono::duration_cast<std::chrono::nanoseconds>(totalEnd - totalStart);
        auto pureElapsed = totalWallClock - m_accumulatedOverheadTime; // m_accumulatedOverheadTime에는 대기 시간이 누적되어 있음
        if (pureElapsed.count() < 0) {
            pureElapsed = std::chrono::nanoseconds(0);
        }

        int pureElapsedMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(pureElapsed).count()
        );

        if (result == SolveResult::Success) {
            emit boardUpdated(m_board);
            // 성공 : elapsedMs 데이터 주입
            emit finished(m_jobType, static_cast<int>(pureElapsedMs));
        } else {
            // 실패/중단: std::unexpected 에러 상태 주입
            emit finished(m_jobType, std::unexpected<SolveResult>(result));
        }
    } else if(m_jobType == JobType::Generate) {
        bool success = solver.generate(m_board, m_difficulty, stopOnlyCallback);
        emit boardUpdated(m_board);

        if (success) {
            // 생성 성공 시 0ms 및 Success 반환
            emit finished(m_jobType, 0);
        } else {
            // 도중에 중단된 경우 Aborted 통보
            emit finished(m_jobType, std::unexpected<SolveResult>(SolveResult::Aborted));
        }
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

    // 1. 일시 정지 (Pause)
    {
        QMutexLocker locker(&m_pauseMutex);
        if(m_isPaused) {
            auto waitStart = std::chrono::steady_clock::now();
    
            while (m_isPaused) {
                if(m_stopRequested) return false;
                m_pauseCondition.wait(&m_pauseMutex);
            }
            m_accumulatedOverheadTime += (std::chrono::steady_clock::now() - waitStart); // 대기 시간 합산
        }
    }

    // 2. UI 갱신 (Throttling)
    if(m_delay > THROTTLING_DELAY_THRESHOLD_MS || m_updateTimer.elapsed() >= UI_UPDATE_INTERVAL_MS) {
        auto emitStart = std::chrono::steady_clock::now();
        emit boardUpdated(currentBoard);
        m_updateTimer.restart();
        m_accumulatedOverheadTime += (std::chrono::steady_clock::now() - emitStart); // 시그널 오버헤드 합산
    }

    // 3. 지연 (Smart Sleep)
    if(m_delay > 0) {
        auto sleepStart = std::chrono::steady_clock::now();
        QMutexLocker locker(&m_pauseMutex);
        // m_delay 만큼 대기하되 requestStop()이 wakeAll()을 부르면 즉시 깨어남
        if(!m_stopRequested) { // 이미 stop 요청이 왔다면 잘 필요 없음
            m_pauseCondition.wait(&m_pauseMutex, m_delay);
        }
        m_accumulatedOverheadTime += (std::chrono::steady_clock::now() - sleepStart); // 슬립 시간 합산
    }

    return true; // 계속 진행
}
