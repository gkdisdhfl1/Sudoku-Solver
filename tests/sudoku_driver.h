#pragma once

#include <QQuickWindow>
#include <QQuickItem>
#include <QTest>
#include <QString>

class SudokuDriver;

namespace sudoku_test {
    // 리피터는 윈도우 라이프타임 도안 파괴되지 않는 불변 컴포넌트
    inline QQuickItem* findRepeater(QQuickWindow* window) {
        if (!window)
            return nullptr;

        auto* grid = window->findChild<QQuickItem*>(QStringLiteral("sudokuGrid"));
        return grid ? grid->findChild<QQuickItem*>(QStringLiteral("cellRepeater")) : nullptr;
    }

    // Repeater.itemAt 호출 헬퍼
    inline QQuickItem* fetchCellFromRepeater(QQuickItem* repeater, int index) {
        if (!repeater || index < 0 || index >= 81)
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
   CellDriver(QQuickWindow* window, QQuickItem* repeater, int index) 
        : m_window(window), m_repeater(repeater), m_index(index) {}

    // 셀의 정체성은 m_index. lookup()을 통해 Repeater에서 '현재' 델리게이트를 매번 조회 (O(1))
    QQuickItem* item() const {
        QQuickItem* it = lookup();
        [&] {
            QVERIFY2(it, qPrintable(QStringLiteral("Cell %1 not found in QML tree").arg(m_index)));
        }();
        return it;
    }

    // 1. 마우스 클릭 (동적 중심 좌표 계산)
    // 클릭 후 해당 셀이 실제로 activeFocus를 획득할 대까지 조건 대기
    CellDriver& click() {
        if (QQuickItem* it = item()) {
            QPointF localCenter(it->width() / 2.0, it->height() / 2.0);
            QPointF scenePos = it->mapToScene(localCenter);
            QTest::mouseClick(m_window, Qt::LeftButton, Qt::NoModifier, scenePos.toPoint());
            const bool ok = QTest::qWaitFor([this] { return isFocused(); });
            [&] {
                QVERIFY2(ok, qPrintable(QStringLiteral("Cell %1 failed to gain active focus after click").arg(m_index)));
            }();
        }
        return *this;
    }

    // 2. 키보드 숫자 문자열 입력
    // QTest::keyClick은 동기로 전달되므로 고정 qWait 없이 즉시 전달 후 이벤트 처리
    // 대상 셀이 포커스를 쥐고 있지 않다면 자동으로 focus() 수행
    CellDriver& type(const QString& text) {
        focus();
        if (item() && m_window) {
            for (const QChar& ch : text) {
                const char ascii = ch.toLatin1();
                [&] {
                    QVERIFY2(
                        ascii != '\0',
                        qPrintable(QStringLiteral("Non-Latin1 character '%1' cannot be typed in CellDriver::type").arg(ch))
                    );
                }();
                QTest::keyClick(m_window, ascii);
                QCoreApplication::processEvents();
            }
        }
        return *this;
    }

    // 3. 단일 키 입력 (방향키, Backsapce, Delete 등). 포커스 된 위치 기준
    // 대상 셀이 포커스를 쥐고 있지 않다면 자동으로 focus() 수행
    CellDriver& press(Qt::Key key) {
        focus();
        if (item() && m_window) {
            QTest::keyClick(m_window, key);
            QCoreApplication::processEvents();
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

        if (!it)
            return false;
        
        auto* input{it->findChild<QQuickItem*>(QStringLiteral("inputField"))};
        return input && input->hasActiveFocus();
    }

    QVariantList candidates() const {
        QQuickItem* it = item();
        return it ? it->property("candidates").toList() : QVariantList();
    }

    int index() const {
        return m_index;
    }

    // QVERIFY 없이 '지금' 셀이 트리에 존재하는지만 반환 (사전 검증/조건 분기용)
    [[nodiscard]] bool exists() const {
        return lookup() != nullptr;
    }

    // 체이닝 연계: 다른 셀로 즉시 전환
    CellDriver cell(int row, int col) const;
    CellDriver cell(int index) const;

    // 포커스를 획득갈 때까지 대기하는 명시적 헬퍼
    CellDriver& waitForFocus(int timeoutMs = 5000) {
        const bool ok = QTest::qWaitFor([this] { return isFocused(); }, timeoutMs);
        [&] {
            QVERIFY2(ok, qPrintable(QStringLiteral("Cell %1 timed out wating for active focus").arg(m_index)));
        }();
        return *this;
    }

    // 특정 셀 값을 가질 때까지 대기하는 명시적 헬퍼
    CellDriver& waitForValue(int expected, int timeoutMs = 5000) {
        const bool ok = QTest::qWaitFor([this, expected] { return value() == expected; }, timeoutMs);
        [&] {
            QVERIFY2(ok, qPrintable(QStringLiteral("Cell %1 timed out waiting for value %2 (current: %3)")
                .arg(m_index).arg(expected).arg(value())));
        }();
        return *this;
    }

    // 대상 셀로 포커스 이동 (이미 활성 포커스를 가지고 있다면 No-op)
    CellDriver& focus() {
        if (!isFocused()) {
            click();
        }
        return *this;
    }

private:
    // Repeater는 윈도우 수명 동안 유지되므로 이것만 캐시. 셀은 정체성이 m_index이므로 캐시하지 않음
    QQuickItem* lookup() const {
        if (!m_repeater && m_window)
            m_repeater = sudoku_test::findRepeater(m_window);
        return sudoku_test::fetchCellFromRepeater(m_repeater, m_index);
    }

    QPointer<QQuickWindow> m_window;
    mutable QPointer<QQuickItem> m_repeater;
    int m_index;
};

// ==============================================================================
// SudokuDriver: 최상위 윈도우 기반 E2E 테스트 오케스트레이터
// ==============================================================================
class SudokuDriver {
public:
    explicit SudokuDriver(QQuickWindow* window) 
        : m_window(window), m_repeater(sudoku_test::findRepeater(window)) {}
    
    static SudokuDriver on(QQuickWindow* window) {
        return SudokuDriver(window);
    }

    // 특정 셀 획득 (row: 0~8, col: 0~8)
    CellDriver cell(int row, int col) const {
        return {m_window, m_repeater ,row * 9 + col};
    }

    // 특정 셀 획득 (1차원 index: 0~80)
    CellDriver cell(int index) const {
        return {m_window, m_repeater, index};
    }

    // 전역 키 입력
    SudokuDriver& press(Qt::Key key) {
        if (m_window) {
            QTest::keyClick(m_window, key);
            QCoreApplication::processEvents();
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
    QPointer<QQuickItem> m_repeater;
};

// CellDriver 인라인 구현
inline CellDriver CellDriver::cell(int row, int col) const{
    return {m_window, m_repeater, row * 9 + col};
}

inline CellDriver CellDriver::cell(int index) const{
    return {m_window, m_repeater, index};
}