#include "sudoku_backend.h"

#include "sudoku_solver.h"

#include "sudoku_constants.h"

#include <QCoreApplication>
#include <QThread>

using namespace SudokuConstants;

SudokuBackend::SudokuBackend(QObject *parent)
    : QAbstractListModel{parent}
{
    m_board.resize(9);
    for(int i{0}; i < 9; ++i) {
        m_board[i].resize(9);
        m_board[i].fill(0);
    }

    // ====================================================
    // 스레드 간 안전한 시그널 전송을 위한 Qt 메타타입 등록
    // ====================================================
    qRegisterMetaType<std::expected<int, SolveResult>>("std::expected<int, SolveResult>");
    qRegisterMetaType<SolverWorker::JobType>("SolverWorker::JobType");
    qRegisterMetaType<StepInfo>("StepInfo");

    // 초기 보드에 대한 후보 캐시 계산
    recalculateAllCandidates();

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
    
    return 81;
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
    } else if (role == ErrorRole) {
        return m_errorCells.test(idx);
    } else if (role == IsTargetRole) {
        return (idx == m_targetIndex);
    } else if (role == CandidatesRole) {
        // 해당 빈 셀에 들어갈 수 있는 1~9 유효 후보 숫자를 즉시 계산
        QVariantList candidateList;
        if (m_board[r][c] == 0) {
            const auto& bits = m_candidateCache[idx];
            for (int num = 1; num <= 9; ++num) {
                if (bits.test(num - 1)) {
                    candidateList.append(num);
                }
            }
        }
        return candidateList;
    }

    return QVariant();
}

QHash<int, QByteArray> SudokuBackend::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ValueRole] = "value";
    roles[ErrorRole] = "isError";
    roles[CandidatesRole] = "candidates";
    roles[IsTargetRole] = "isTarget";
    return roles;
}

bool SudokuBackend::hasErrors() const
{
    // 에러가 1개라도 존재하는지 여부 반환
    return m_errorCells.any();
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

SudokuSolver::SolveAlgorithm SudokuBackend::algorithm() const
{
    return m_algorithm;
}

void SudokuBackend::setAlgorithm(SudokuSolver::SolveAlgorithm algo)
{
    if (m_algorithm != algo) {
        m_algorithm = algo;
        emit algorithmChanged();
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

QString SudokuBackend::statusMessage() const
{
    return m_statusMessage;
}

void SudokuBackend::setCell(int cellIndex, int value)
{
    if(m_isBusy) return; // 작업 중엔 수정 불가

    if(cellIndex < 0 || cellIndex >= 81) return;
    int r = cellIndex / 9;
    int c = cellIndex % 9;

    if(value < 0 || value > 9) return;

    if(m_board[r][c] != value) {
        m_board[r][c] = value;

        updatePeerCandidatesAt(r, c);

        // 표준 index() API로 인덱스를 생성하고, 역할 필터 없이 확실하게 dataChanged 발행
        QModelIndex modelIdx = index(cellIndex, 0);
        emit dataChanged(modelIdx, modelIdx);
        // emit dataChanged(index(0, 0), index(80, 0), {ValueRole, CandidatesRole});

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

    recalculateAllCandidates();
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
    if (m_worker) {
        m_worker->requestStop();
    }
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }

    m_isBusy = true;
    emit isBusyChanged();

    m_workerThread = new QThread;

    // JobType에 따라 인자가 다르지만, SolverWorker 생성자 활용
    bool visualize = (jobType == SolverWorker::JobType::Solve) ? m_visualize : false;
    int delay = (jobType == SolverWorker::JobType::Solve) ? m_delay : 0;

    // Worker 생성 (Solve 모드)
    m_worker = new SolverWorker(m_board, jobType, visualize, delay, difficulty, m_algorithm);
    m_worker->moveToThread(m_workerThread);

    // 시그널 연결
    connect(m_workerThread, &QThread::started, m_worker, &SolverWorker::process);
    connect(m_worker, &SolverWorker::stepUpdated, this, &SudokuBackend::handleStepUpdate);
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
        emit solveFinished(1, 0); // 실패 상태코드 1, 시간 0ms 통보
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

    // 워커의 멤버 함수를 메인 스레드에서 직접 호출
    // setPaused() 내부는 QMutexLocker에 의해 보호되며,
    // 일시정지 해제 시 wakeAll()로 블로킹 중인 워커 스레드를 즉시 깨워야 하므로
    // QueuedConnection이 아닌 직접 호출이 의도적으로 필요함.
    m_worker->setPaused(m_isPaused);

    emit isPausedChanged();
}

// -- handler ---
void SudokuBackend::handleWorkerFinished(SolverWorker::JobType jobType, std::expected<int, SolveResult> result)
{
    m_isBusy = false;
    m_isPaused = false;
    m_targetIndex = -1;
    m_statusMessage.clear();
    emit isBusyChanged();
    emit isPausedChanged();
    emit statusMessageChanged();

    // 최종 상태 캐시 동기화 및 뷰 전체 갱신
    recalculateAllCandidates();

    // 전체 역할 갱신 (IsTargetRole 포함)
    emit dataChanged(index(0, 0), index(80, 0));

    m_worker = nullptr;
    m_workerThread = nullptr; // deleteLater로 삭제되므로 포인터만 null 처리

    checkErrors(); // 최종 상태 에러 검사

    if (jobType == SolverWorker::JobType::Solve) {
        if (result.has_value()) {
            emit solveFinished(0, result.value()); // 성공(0) 및 측정 시간 전달
        } else {
            emit solveFinished(static_cast<int>(result.error()), 0); // 실패 코드(1 or 2) 및 0ms 전달
        }
    } else if (jobType == SolverWorker::JobType::Generate) {
        if (result.has_value()) {
            emit generateFinished(true); 
        } else {
            emit generateFinished(false);
        }
    }
}

void SudokuBackend::handleStepUpdate(const StepInfo& info)
{
    // 1. 타겟 인덱스 및 MRV 상대 텍스트 동기화
    m_targetIndex = (info.targetRow >= 0 && info.targetCol >= 0)
                    ? (info.targetRow * 9 + info.targetCol)
                    : -1;
    
    // 2. 메시지 생성 및 반영
    QString newMsg;
    if (!info.extraMessage.isEmpty()) {
        newMsg = info.extraMessage;  
    } else if (info.targetRow >= 0 && info.targetCol >= 0 && !info.candidates.isEmpty()) {
        // 원시 데이터(행/열/후보)를 바탕으로 UI 문자열 가공
        QStringList strList;
        for (int num : info.candidates)
            strList << QString::number(num);
        newMsg = QString("Cell (%1, %2) ➔ Candidates: [%3]")
            .arg(info.targetRow + 1)
            .arg(info.targetCol + 1)
            .arg(strList.join(", "));
    }

    if (m_statusMessage != newMsg) {
        m_statusMessage = newMsg;
        emit statusMessageChanged();
    }

    // 3. 보드 데이터 비교 및 20여 개 주변 셀 캐시 갱신
    std::vector<int> changedIndices;
    for(int r{0}; r < 9; ++r) {
        if (m_board[r] == info.board[r])
            continue;
        for (int c{0}; c < 9; ++c) {
            if (m_board[r][c] != info.board[r][c]) {
                m_board[r][c] = info.board[r][c];
                changedIndices.push_back(r * 9 + c);
            }
        }
    }

    // [캐시 갱신 및 불변성 설계 의도]
    // 1. 시각화 모드(m_visualize == true):
    //   - 매 스텝마다 변경된 셀 주변 20여 개 셀의 후보 캐시를 증분 갱신하여 3x3 노트를 실시간 반영함.
    // 2. 비시각화 모드(m_visualize == false):
    //   - 풀이 속도를 위해 중간 스텝의 캐시 계산을 의도적으로 생략함.
    //   - 최종 보드의 후보 캐시 동기화는 작업 완료 시 handleWorkerFinished()의
    //     recalculateAllCandidate()에서 100% 보장되므로 데이터 일관성이 안전하게 유지됨.
    if (!changedIndices.empty() && m_visualize) {
        std::bitset<81> updatedMask;
        for (int idx : changedIndices) {
           updatePeerCandidatesAt(idx / 9, idx % 9, &updatedMask);
        }
    }

    // 4. 보드 숫자, 연필 자국, 타겟 하이라이트를 동시 갱신
    emit dataChanged(index(0, 0), index(80, 0), {ValueRole, CandidatesRole, IsTargetRole});
}

//  --- private ---
void SudokuBackend::checkErrors()
{
    std::bitset<81> newErrors;
    for(int r = 0; r < 9; ++r) {
        for(int c = 0; c < 9; ++c) {
            int num = m_board[r][c];
            if(num != 0) {
                if(!SudokuSolver::isValid(m_board, r, c, num, true)) {
                    newErrors.set(r * 9 + c);
                }
            }
        }
    }

    if(m_errorCells != newErrors) {
        // 1. XOR 연산 한 번으로 변경된 인덱스 비트 필드  획득 (힙 메모리 할당 0)
        std::bitset<81> changed = m_errorCells ^ newErrors;
        bool wasErrors = m_errorCells.any();

        m_errorCells = newErrors;

        // 2. 전체 에러 보유 상태가 변경되었을 때만 프로퍼티 신호 방출
        if(wasErrors != m_errorCells.any()) {
            emit hasErrorsChanged();
        }

        // 3. 상태가 바뀐(1이 켜진) 인덱스에 대해서만 ErrorRole 업데이트 통보
        for(size_t idx = 0; idx < 81; ++idx) {
            if(changed.test(idx)) {
                QModelIndex modelIdx = index(idx, 0);
                emit dataChanged(modelIdx, modelIdx, {ErrorRole});
            }
        }
    }
}

void SudokuBackend::recalculateAllCandidates()
{
    for(int r{0}; r < 9; ++r) {
        for(int c{0}; c < 9; ++c) {
            updateCandidateCacheAt(r, c);
        }
    }
}

void SudokuBackend::updateCandidateCacheAt(int r, int c)
{
    int idx{r * 9 + c};
    m_candidateCache[idx].reset();

    if (m_board[r][c] == 0) {
        for (int num{1}; num <= 9; ++num) {
            if (SudokuSolver::isValid(m_board, r, c, num)) {
                m_candidateCache[idx].set(num - 1); // 1~9 숫자를 0~8 비트에 저장
            }
        }
    }
}

void SudokuBackend::updatePeerCandidatesAt(int r, int c, std::bitset<81>* updatedMask)
{
    int boxStartR{r - r % 3};
    int boxStartC{c - c % 3};

    // 외부에서 마스크를 안 주면 내부에서 로컬 마스크 생성
    std::bitset<81> localMask;

    // 이미 방문한 셀은 건너뛰고 최초 1회만 계산
    auto updateOnce = [this, updatedMask](int row, int col) {
        int idx{row * 9 + col};
        if (!updatedMask || !updatedMask->test(idx)) {
            if (updatedMask)
                updatedMask->set(idx);
            updateCandidateCacheAt(row, col);
        }
    };

    // 1. 해당 셀 자체 캐시 갱신
    updateOnce(r, c);

    // 2. 동일 행 및 동일 열 캐시 갱신
    for (int i{0}; i < 9; ++i) {
        updateOnce(r, i);
        updateOnce(i, c);
    }

    // 3. 동일 3x3 박스 캐시 갱신
    for (int br{0}; br < 3; ++br) {
        for (int bc{0}; bc < 3; ++bc) {
            updateOnce(boxStartR + br, boxStartC + bc);
        }
    }
}
