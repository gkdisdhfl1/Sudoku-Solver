#ifndef SOLVER_WORKER_H
#define SOLVER_WORKER_H

#include <QObject>
#include <QElapsedTimer>
#include <QMutex>
#include <QWaitCondition>
#include <expected>
#include <chrono>
#include "sudoku_solver.h"

class SolverWorker : public QObject
{
    Q_OBJECT
public:
    // 풀기 모드 또는 생성 모드 구분을 위한 타입
    enum class JobType { Solve, Generate };

    explicit SolverWorker(const QVector<QVector<int>> &board,
                          JobType jobType,
                          bool visualize,
                          int delay,
                          int difficulty = 0,
                          SudokuSolver::SolveAlgorithm algorithm = SudokuSolver::SolveAlgorithm::BackTracking,
                          QObject *parent = nullptr);

    JobType jobType() const {
        return m_jobType;
    }

public slots:
    void process(); // 스레드 시작 시 호출될 메인 함수
    void requestStop(); // 외부에서 중단 요청
    void setPaused(bool paused); // 일시 정지 제어

signals:
    void boardUpdated(const QVector<QVector<int>> &board);
    // 성공 시 풀이 시간(int), 실패 시 예외 결과(SolveResult) 전송
    void finished(SolverWorker::JobType jobType, std::expected<int, SolveResult> result);
    void mrvStatusUpdated(int r, int c, const QVector<int>& candidates);

private:
    QVector<QVector<int>> m_board;
    JobType m_jobType;
    bool m_visualize;
    int m_delay;
    int m_difficulty;
    SudokuSolver::SolveAlgorithm m_algorithm;
    std::atomic_bool m_stopRequested{false};
    QElapsedTimer m_updateTimer;

    // 알고리즘 외 UI 대기 및 지연에 소요된 총 오버헤드 시간
    std::chrono::nanoseconds m_accumulatedOverheadTime{0};

    // 일시 정지 관련
    QMutex m_pauseMutex;
    QWaitCondition m_pauseCondition;
    std::atomic_bool m_isPaused{false};

    bool processStep(const QVector<QVector<int>>& currentBoard);
};

#endif // SOLVER_WORKER_H
