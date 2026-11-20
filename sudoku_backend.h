#ifndef SUDOKU_BACKEND_H
#define SUDOKU_BACKEND_H

#include "solver_worker.h"
#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QPointer>
#include <bitset>
#include <expected>
#include "sudoku_solver.h"

class SudokuBackend : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasErrors READ hasErrors NOTIFY hasErrorsChanged);

    // 시각화 관련 프로퍼티
    Q_PROPERTY(bool visualize READ visualize WRITE setVisualize NOTIFY visualizeChanged)
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged )
    Q_PROPERTY(SudokuSolver::SolveAlgorithm algorithm READ algorithm WRITE setAlgorithm NOTIFY algorithmChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged ) // 작업 중 여부 표시
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY isPausedChanged)

    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(int stepState READ stepState NOTIFY stepInfoChanged)
    Q_PROPERTY(int targetRow READ targetRow NOTIFY stepInfoChanged)
    Q_PROPERTY(int targetCol READ targetCol NOTIFY stepInfoChanged)
    Q_PROPERTY(int targetValue READ tryingValue NOTIFY stepInfoChanged)
    Q_PROPERTY(QVariantList targetCandidates READ targetCandidates NOTIFY stepInfoChanged)

    QML_ELEMENT // QML에서 직접 사용할 수 있게 등록
public:
    explicit SudokuBackend(QObject *parent = nullptr);
    ~SudokuBackend(); // 소멸자 추가 (스레드 정리용)

    // QAbstractListModel 인터페이스 오버라이드
    enum BoardRoles {
        ValueRole = Qt::UserRole + 1,
        ErrorRole,
        CandidatesRole, // 1~9 유효 후보 숫자 목록
        IsTargetRole,   // 알고리즘이 주시 중인 타겟 셀 여부
        IsGivenRole,    // 초기 문제(사용자/생성) 숫자 여부
        StepStateRole,  // 타겟 셀 단계 상태 (StepState)
        TryingValueRole // 시도 중/백트래킹 중인 숫자
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool hasErrors() const;

    // 시각화 Getter/Setter
    bool visualize() const;
    int delay() const;
    SudokuSolver::SolveAlgorithm algorithm() const;
    bool isBusy() const;
    bool isPaused() const;
    QString statusMessage() const;
    int stepState() const { return static_cast<int>(m_stepState); }
    int targetRow() const { return m_targetIndex >= 0 ? (m_targetIndex / 9) : -1; }
    int targetCol() const { return m_targetIndex >= 0 ? (m_targetIndex % 9) : -1; }
    int tryingValue() const { return m_tryingValue; };
    QVariantList targetCandidates() const;

    void setVisualize(bool v);
    void setDelay(int d);
    void setAlgorithm(SudokuSolver::SolveAlgorithm algo);


    Q_INVOKABLE void setCell(int cellIndex, int value);
    // Q_INVOKABLE int getCell(int index) const;
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool isValidBoard() const;

    // 메인 기능
    Q_INVOKABLE void solve();
    Q_INVOKABLE void generatePuzzle(int difficulty = 0);
    Q_INVOKABLE void stop(); // 중단
    Q_INVOKABLE void togglePause();

signals:
    void hasErrorsChanged();
    void visualizeChanged();
    void delayChanged();
    void algorithmChanged();
    void isBusyChanged();
    void isPausedChanged();
    void solveFinished(int status, int elapsedMs);
    void generateFinished(bool success);
    void statusMessageChanged();
    void stepInfoChanged();

private slots:
    // 워커 시그널 처리용
    void handleStepUpdate(const StepInfo& info);
    void handleWorkerFinished(SolverWorker::JobType jobType, std::expected<int, SolveResult> result);

private:
    QVector<QVector<int>> m_board;                      // 0~80, 0 means empty
    std::bitset<81> m_errorCells;
    std::array<std::bitset<9>, 81> m_candidateCache;    // 81개 셀 유효 후보 비트셋 캐시
    std::bitset<81> m_isGiven;                          // 초기 문제 숫자 마스크
    int m_targetIndex{-1};                              // 현재 알고리즘이 주시 중인 1차원 셀 인덱스
    StepState m_stepState {StepState::None};
    int m_tryingValue{0};
    QVector<int> m_targetCandidates;

    // 시각화 설정 변수
    bool m_visualize{false};
    int m_delay{50};                                    // 기본값 50ms
    bool m_isBusy{false}; // 작업 중 상태
    bool m_isPaused{false};
    SudokuSolver::SolveAlgorithm m_algorithm{SudokuSolver::SolveAlgorithm::BackTracking};
    QString m_statusMessage;

    // 스레드 관련
    QPointer<QThread> m_workerThread{nullptr};
    QPointer<SolverWorker> m_worker{nullptr};

    void checkErrors();                                                                     // 에러 검사 수행 및 리스트 업데이트
    void startWorker(SolverWorker::JobType jobType, int difficulty = 0);                    // 공통 워커 시작 함수
    void recalculateAllCandidates();                                                        // 전체 후보 캐시 재계산 헬퍼
    void updateCandidateCacheAt(int r, int c);                                              // 특정 셀의 후보 비트셋 캐시 갱신 헬퍼
    void updatePeerCandidatesAt(int r, int c, std::bitset<81>* updatedMask = nullptr);      // 특정 셀과 연관된 20여 개 셀 캐시 증분 갱신
    void syncGivenCellsFromBoard();                                                         // 현재 보드의 0이 아닌 셀을 초기 문제 숫자로 마킹
};

#endif // SUDOKU_BACKEND_H
