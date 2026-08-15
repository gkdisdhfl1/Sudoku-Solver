#ifndef SUDOKU_SOLVER_H
#define SUDOKU_SOLVER_H

#include <QVector>
#include <functional>
#include <random>
#include <QObject>
#include <qtqml/qqmlregistration.h>

// 성공 실패, 사용자에 의한 중단을 구분하기 위한 리턴 타입 정의
enum class SolveResult {
    Success,
    Failed,
    Aborted
};

// =======================================================================
// StepInfo: 탐색 단계 상태 콜백으로 전달 및 스레드 간 시그널 전송을 위한 통합 데이터 객체
//
// [주의: 수명 제약 (Lifetime Constraint)]
// - 'board' 멤버는 원본 보드에 대한 const 참조자(Non-owning Reference)임.
// - 본 구조체는 StepCallback의 동기식 호출 스코프 내에서만 유효함.
// - StepInfo 인스턴스를 멤버 변수에 보관하거나 다른 스레드로 비동기 전달할 경우
//   댕글링 참조 (Dangling Reference)가 발생하므로, 반드시 보드를 복사하여 저장해야 함.
// =======================================================================
struct StepInfo {
    QVector<QVector<int>> board;         // 필수: 현재 9x9 보드 상태
    int targetRow{-1};                          // 현재 주목 중인 행
    int targetCol{-1};                          // 현재 주목 중인 열
    QVector<int> candidates{};                  // MRV 등 후보 숫자 목록
    QString extraMessage{};                     // 향후 DLX 등 타 알고리즘용 상용 텍스트
};
Q_DECLARE_METATYPE(StepInfo)

using StepCallback = std::function<bool(const StepInfo& info)>;

class SudokuSolver
{
    Q_GADGET
    QML_ELEMENT
    
public:
    enum class SolveAlgorithm {
        BackTracking,
        Randomly,
        MRV, // Minimum Remaining Values 휴리스틱
    };
    Q_ENUM(SolveAlgorithm)

    explicit SudokuSolver();

    // 유효성 검사
    static bool isValid(const QVector<QVector<int>> &board, int r, int c, int num, bool ignoreSelf = false);

    // 백트래킹 풀이
    // callback이 nullptr이면 일반 실행, 있으면 매 단계 호출
    SolveResult solve(QVector<QVector<int>> &board,
               SolveAlgorithm algorithm = SolveAlgorithm::BackTracking,
               StepCallback callback = nullptr);

    // 퍼즐 생성
    bool generate(QVector<QVector<int>> &board, int difficulty, StepCallback callback = nullptr);

    // 유일해 검증 함수
    bool hasUniqueSolution(QVector<QVector<int>>& board);

private:
    SolveResult solveBacktracking(QVector<QVector<int>> &board, StepCallback callback, int idx = 0);

    SolveResult solveRandomly(QVector<QVector<int>> &board, StepCallback callback, int idx = 0);

    SolveResult solveMRV(QVector<QVector<int>>& board, StepCallback callback = nullptr);

    // 해의 개수를 세기 위한 재귀 백트래킹 함수 (최대 2개까지만 카운트)
    int countSolutions(QVector<QVector<int>>& board, int maxCount, int idx = 0);

    static std::mt19937& get_thread_local_generator();
};

#endif // SUDOKU_SOLVER_H
