#ifndef SUDOKU_BACKEND_H
#define SUDOKU_BACKEND_H

#include "solver_worker.h"
#include <QObject>
#include <QtQml/qqmlregistration.h>

class SudokuBackend : public QObject
{
    Q_OBJECT
    // QML에서 접근할 때 사용할 1차원 형태의 프로퍼티
    Q_PROPERTY(QList<int> board READ board NOTIFY boardChanged)
    Q_PROPERTY(QList<int> errorCells READ errorCells NOTIFY errorCellsChanged)

    // 시각화 관련 프로퍼티
    Q_PROPERTY(bool visualize READ visualize WRITE setVisualize NOTIFY visualizeChanged)
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged )
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged ) // 작업 중 여부 표시
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY isPausedChanged)

    QML_ELEMENT // QML에서 직접 사용할 수 있게 등록
public:
    explicit SudokuBackend(QObject *parent = nullptr);
    ~SudokuBackend(); // 소멸자 추가 (스레드 정리용)

    QList<int> board() const;
    QList<int> errorCells() const;

    // 시각화 Getter/Setter
    bool visualize() const;
    int delay() const;
    bool isBusy() const;
    bool isPaused() const;

    void setVisualize(bool v);
    void setDelay(int d);


    Q_INVOKABLE void setCell(int index, int value);
    // Q_INVOKABLE int getCell(int index) const;
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool isValidBoard() const;

    // 메인 기능
    Q_INVOKABLE void solveBacktracking();
    Q_INVOKABLE void generatePuzzle(int difficulty = 0);
    Q_INVOKABLE void stop(); // 중단
    Q_INVOKABLE void togglePause();

signals:
    void boardChanged();
    void errorCellsChanged();
    void visualizeChanged();
    void delayChanged();
    void isBusyChanged();
    void isPausedChanged();
    void solveFinished(bool success);

private slots:
    // 워커 시그널 처리용
    void handleBoardUpdate(const QVector<QVector<int>> &board);
    void handleWorkerFinished(bool success);

private:
    QVector<QVector<int>> m_board; // 0~80, 0 means empty
    QList<int> m_errorCells;

    // 시각화 설정 변수
    bool m_visualize{false};
    int m_delay{50}; // 기본값 50ms
    bool m_isBusy{false}; // 작업 중 상태
    bool m_isPaused{false};

    // 스레드 관련
    QThread* m_workerThread{nullptr};
    SolverWorker* m_worker{nullptr};

    void checkErrors(); // 에러 검사 수행 및 리스트 업데이트
};

#endif // SUDOKU_BACKEND_H
