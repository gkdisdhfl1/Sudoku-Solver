#pragma once

#include <QQuickWindow>
#include <QQuickItem>
#include <QtTest>
#include <QTest>
#include <QString>

class SudokuDriver;

namespace sudoku_test {
    // objectName 기반 조회 (휴리스틱 fallback 제거), 매 호출마다 현재 델리게이트를 반환
    inline QQuickItem* findCellItem(QQuickWindow* window, int index) {
        if (!window || index < 0 || index >= 81)
            return nullptr;

        auto* grid = window->findChild<QQuickItem*>(QStringLiteral("sudokuGrid"));
        auto* repeater = grid ? grid->findChild<QQuickItem*>(QStringLiteral("cellRepeater")) : nullptr;
        if (!repeater)
            return nullptr;

        QQuickItem* cell = nullptr;
        const bool ok = QMetaObject::invokeMethod(repeater, "itemAt", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem*, cell), Q_ARG(int, index));

        if (!ok)
            qWarning("Repeater.itemAt invocation failed (method renamed or signature changed?)");
        return cell;
    }
} // namespace sudoku_test

// ==============================================================================
// CellDriver: 특정 셀에 바인딩된 Fluent Action Chaining Class
// ==============================================================================
class CellDriver {
public:
   CellDriver(QQuickWindow* window, int index) : m_window(window), m_index(index) {}

    // 셀의 정체성은 m_index. 포인터를 캐시하지 않고 매번 조회 -> 모델 리셋 이후에도 유효
    // 반환값을 변수에 저장하지 말 것 (clear() 등으로 델리게이트가 재생성되면 무효화됨).
    // 셀을 찾지 못하면 실패로 기록한다. QTest::setThrowOnFail(true)가 켜져 있으면
    // 예외로 테스트 함수가 즉시 중단되며, 그렇지 않으면 nullptr을 반환한다.
    QQuickItem* item() const {
        QQuickItem* it = sudoku_test::findCellItem(m_window, m_index);
        // QVERIFY는 'return';으로 확장되므로 void 람다로 감쌈
        [&] { QVERIFY2(it, qPrintable(QStringLiteral("Cell %1 not found in QML tree").arg(m_index))); }();
        return it;
    }

    // 1. 마우스 클릭 (동적 중심 좌표 계산)
    CellDriver& click() {
        if (QQuickItem* it = item()) {
            QPointF localCenter(it->width() / 2.0, it->height() / 2.0);
            QPointF scenePos = it->mapToScene(localCenter);
            QTest::mouseClick(m_window, Qt::LeftButton, Qt::NoModifier, scenePos.toPoint());
            QTest::qWait(20);
        }
        return *this;
    }

    // 2. 키보드 숫자 문자열 입력
    // 주의: 키는 윈도우로 전달되므로 현재 포커스된 위치에 입력된다
    // item() 호출은 셀 존재 여부 확인용
    CellDriver& type(const QString& text) {
        if (item() && m_window) {
            for (const QChar& ch : text) {
                QTest::keyClick(m_window, ch.toLatin1());
                QTest::qWait(10);
            }
            QTest::qWait(20);
        }
        return *this;
    }

    // 3. 단일 키 입력 (방향키, Backsapce, Delete 등). 포커스 된 위치 기준
    CellDriver& press(Qt::Key key) {
        if (item() && m_window) {
            QTest::keyClick(m_window, key);
            QTest::qWait(20);
        }
        return *this;
    }

    // 4. 셀 상태 검증 헬퍼 (Assertions)
    int value() const {
        QQuickItem* it = item();
        return it ? it->property("value").toInt() : -1;
    }

    bool isError() const {
        QQuickItem* it = item();
        return it ? it->property("isError").toBool() : false;
    }

    bool isFocused() const {
        QQuickItem* it = item();

        // SudokuCell 내부의 iputField가 실제 키보드 포커스를 쥐고 있는지 검증
        QQuickItem* input = it ? it->findChild<QQuickItem*>(QStringLiteral("inputField")) : nullptr;
        return input && input->hasActiveFocus();
    }

    QVariantList candidates() const {
        QQuickItem* it = item();
        return it ? it->property("candidates").toList() : QVariantList();
    }

    int index() const {
        return m_index;
    }

    // 체이닝 연계: 다른 셀로 즉시 전환
    CellDriver cell(int row, int col) const;
    CellDriver cell(int index) const;

private:
    QPointer<QQuickWindow> m_window; // 창은 테스트 케이스 동안 정체성이 바뀌지 않으므로 이것만 유지
    int m_index;
};

// ==============================================================================
// SudokuDriver: 최상위 윈도우 기반 E2E 테스트 오케스트레이터
// ==============================================================================
class SudokuDriver {
public:
    explicit SudokuDriver(QQuickWindow* window) : m_window(window) {}
    
    static SudokuDriver on(QQuickWindow* window) {
        return SudokuDriver(window);
    }

    // 특정 셀 획득 (row: 0~8, col: 0~8)
    CellDriver cell(int row, int col) const {
        return {m_window, row * 9 + col};
    }

    // 특정 셀 획득 (1차원 index: 0~80)
    CellDriver cell(int index) const {
        return {m_window, index}; // 생성 시점에는 조회하지 않음
    }

    // 전역 키 입력
    SudokuDriver& press(Qt::Key key) {
        if (m_window) {
            QTest::keyClick(m_window, key);
            QTest::qWait(20);
        }
        return *this;
    }

    // 대기 (UI 애니메이션 및 비동기 처리 대기)
    SudokuDriver& wait(int ms = 50) {
        QTest::qWait(ms);
        return *this;
    }

private:
    QQuickWindow* m_window;
};

// CellDriver 인라인 구현
inline CellDriver CellDriver::cell(int row, int col) const{
    return {m_window, row * 9 + col};
}

inline CellDriver CellDriver::cell(int index) const{
    return {m_window, index};
}