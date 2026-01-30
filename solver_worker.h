#ifndef SOLVER_WORKER_H
#define SOLVER_WORKER_H

#include <QObject>
#include <QElapsedTimer>
#include <QMutex>
#include <QWaitCondition>

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
                          QObject *parent = nullptr);

public slots:
    void process(); // 스레드 시작 시 호출될 메인 함수
    void requestStop(); // 외부에서 중단 요청
    void setPaused(bool paused); // 일시 정지 제어

signals:
    void boardUpdated(const QVector<QVector<int>> &board);
    void finished(bool success);

private:
    QVector<QVector<int>> m_board;
    JobType m_jobType;
    bool m_visualize;
    int m_delay;
    int m_difficulty;
    std::atomic_bool m_stopRequested{false};
    QElapsedTimer m_updateTimer;

    // 시각화가 포함된 백트래킹
    bool solveWithVisualization(QVector<QVector<int>> &board);

    // 일시 정지 관련
    QMutex m_pauseMutex;
    QWaitCondition m_pauseCondition;
    std::atomic_bool m_isPaused{false};
};

#endif // SOLVER_WORKER_H
