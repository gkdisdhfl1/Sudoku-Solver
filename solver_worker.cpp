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
        m_pureAlgorithmTime = std::chrono::nanoseconds(0);

        if(m_visualize) {
            m_updateTimer.start(); // 타이머 시작

            // 콜백을 래퍼로 감싸서 타이머를 자동 정지/재개
            auto timeCallback = [this](const QVector<QVector<int>>& board) -> bool {
                // 알고리즘이 콜백을 호출한 순간 = 알고리즘 구간 끝
                auto callbackEnter = std::chrono::steady_clock::now();
                m_pureAlgorithmTime += (callbackEnter - m_lastResumeTime);

                bool continueRunning = processStep(board);

                // 콜백 반환 직전 = 알고리즘 구간 시작
                m_lastResumeTime = std::chrono::steady_clock::now();
                return continueRunning;
            };
            
            // 최초 알고리즘 시작 시점 기록
            m_lastResumeTime = std::chrono::steady_clock::now();
            result = solver.solve(m_board, SolveAlgorithm::BackTracking, timeCallback);

            // 마지막 알고리즘 구간 (최종 콜백 이후 ~ solve 반환)
            m_pureAlgorithmTime += (std::chrono::steady_clock::now() - m_lastResumeTime);

            emit boardUpdated(m_board);
        } else {
            // 비시각화: 콜백 오버헤드가 거의 없으므로 단순 측정
            auto start = std::chrono::steady_clock::now();
            result = solver.solve(m_board, SolveAlgorithm::BackTracking, stopOnlyCallback);

            m_pureAlgorithmTime = std::chrono::steady_clock::now() - start;
        }

        int pureElapsedMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(m_pureAlgorithmTime).count()
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
        solver.generate(m_board, m_difficulty);
        emit boardUpdated(m_board);
        // 생성 성공 시 더미 값 (0ms) 반환
        emit finished(m_jobType, 0);
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

    // 2. UI 갱신 (Throttling)
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
