#ifndef SUDOKU_CONSTANTS_H
#define SUDOKU_CONSTANTS_H

namespace SudokuConstants {
    // 퍼즐 생성 및 난이도 관련 상수
    constexpr int BASE_REMOVE_COUNT = 32;               // Easy 기본 지우기 타겟 개수
    constexpr int DIFFICULTY_STEP = 10;                 // 난이도 단계별 추가 지우기 개수
    constexpr int MIN_REMOVE_COUNT = 0;                 // 최소 지우기 경계
    constexpr int MAX_REMOVE_COUNT = 64;                // 최대 지우기 경계 (유일해 수학적 한계)

    // 유일해 검증 로직 상수
    constexpr int UNIQUE_SOLUTION_LIMIT = 2;            // 해가 2개 이상이면 유일해 아님 (조기 탈출 기준)

    // 스레드 및 UI 갱신 성능 제어 상수
    constexpr int UI_UPDATE_INTERVAL_MS = 16;           // UI 갱신 타이머 스로틀링 (약 60FPS)
    constexpr int THROTTLING_DELAY_THRESHOLD_MS = 10;   // 강제 갱신 딜레이 임계값
}

#endif // SUDOKU_CONSTANTS_H
