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
            // 람다 함수로 시각화 로직 주입
            success = solver.solve(m_board, SolveAlgorithm::BackTracking,
                                   [this](const QVector<QVector<int>>& currentBoard) -> bool {
                                       if(m_stopRequested) return false; // 중단 요청

                                       emit boardUpdated(currentBoard);
                                       QThread::msleep(m_delay);

                                        return true; // 계속 진행
                                    }
                        );
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
