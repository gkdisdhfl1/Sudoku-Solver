#pragma once

#include <QQuickWindow>
#include <QQuickItem>
#include <QtTest>
#include <QTest>
#include <QString>

class SudokuDriver;

// ==============================================================================
// CellDriver: 특정 셀에 바인딩된 Fluent Action Chaining Class
// ==============================================================================
class CellDriver {
public:
   CellDriver(SudokuDriver& parent, QQuickWindow* window, QQuickItem* cellItem, int index)
            : m_parent(parent), m_window(window), m_cellItem(cellItem), m_index(index) {}

    // 1. 마우스 클릭 (동적 중심 좌표 계산)
    CellDriver& click() {
        if (m_cellItem && m_window) {
            // 창 크기나 마진이 바뀌어도 안전한 중심 좌표 계산
            QPointF localCenter(m_cellItem->width() / 2.0, m_cellItem->height() / 2.0);
            QPointF scenePos = m_cellItem->mapToScene(localCenter);
            QTest::mouseClick(m_window, Qt::LeftButton, Qt::NoModifier, scenePos.toPoint());
            QTest::qWait(20); // 이벤트 루프 처리 대기
        }
        return *this;
    }

    // 2. 키보드 숫자 문자열 입력 (QTest::keyClicks)
    CellDriver& type(const QString& text) {
        if (m_window) {
            for (const QChar& ch : text) {
                QTest::keyClick(m_window, ch.toLatin1());
                QTest::qWait(10);
            }
            QTest::qWait(20);
        }
        return *this;
    }

    // 3. 단일 키 입력 (방향키, Backsapce, Delete 등) 
    CellDriver& press(Qt::Key key) {
        if (m_window) {
            QTest::keyClick(m_window, key);
            QTest::qWait(20);
        }
        return *this;
    }

    // 4. 셀 상태 검증 헬퍼 (Assertions)
    int value() const {
        return m_cellItem ? m_cellItem->property("value").toInt() : -1;
    }

    bool isError() const {
        return m_cellItem ? m_cellItem->property("isError").toBool() : false;
    }

    bool isFocused() const {
        if (!m_cellItem) return false;
        // SudokuCell 내부의 iputField가 실제 키보드 포커스를 쥐고 있는지 검증
        QQuickItem* input = m_cellItem->findChild<QQuickItem*>("inputField");
        if (input) {
            return input->property("activeFocus").toBool();
        }
        return m_cellItem->property("isFocusedCell").toBool();
    }

    QVariantList candidates() const {
        return m_cellItem ? m_cellItem->property("candidates").toList() : QVariantList();
    }

    QQuickItem* item() const {
        return m_cellItem;
    }

    int index() const {
        return m_index;
    }

    // 체이닝 연계: 다른 셀로 즉시 전환
    CellDriver cell(int row, int col);
    CellDriver cell(int index);

private:
    SudokuDriver& m_parent;
    QQuickWindow* m_window;
    QQuickItem* m_cellItem;
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
    CellDriver cell(int row, int col) {
        return cell(row * 9 + col);
    }

    // 특정 셀 획득 (1차원 index: 0~80)
    CellDriver cell(int index) {
        QQuickItem* cellItem = findCellItem(index);
        return CellDriver(*this, m_window, cellItem, index);
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

    // QML 트리에서 특정 인덱스의 SudokuCell 탐색
    QQuickItem* findCellItem(int index) {
        if (!m_window || index < 0 || index >= 81) return nullptr;

        // 1. sudokuGrid 아이템 탐색
        QQuickItem* grid = m_window->findChild<QQuickItem*>("sudokuGrid");
        if (!grid) {
            // objectName이 없을 경우 focusedIndex 속성을 보유한 그리드를 순회 탐색
            const auto items = m_window->contentItem()->findChildren<QQuickItem*>();
            for (auto* item : items) {
                if (item->property("focusedIndex").isValid()) {
                    grid = item;
                    break;
                }
            }
        }

        // 2. cellRepeater를 통해 해당 인덱스의 셀 아이템 획득
        if (grid) {
            QQuickItem* repeater = grid->findChild<QQuickItem*>("cellRepeater");
            if (!repeater) {
                // 이름이 지정되지 않은 경우 자식 Repeater 탐색
                const auto children = grid->findChildren<QQuickItem*>();
                for (auto* child : children) {
                    if (child->property("model").isValid()) {
                        repeater = child;
                        break;
                    }
                }
            }

            if (repeater) {
                QQuickItem* cell = nullptr;
                QMetaObject::invokeMethod(repeater, "itemAt",
                    Qt::DirectConnection,
                    Q_RETURN_ARG(QQuickItem*, cell),
                    Q_ARG(int, index));
                return cell;
            }
        }
        return nullptr;
    }

private:
    QQuickWindow* m_window;
};

// CellDriver 인라인 구현
inline CellDriver CellDriver::cell(int row, int col) {
    return m_parent.cell(row, col);
}

inline CellDriver CellDriver::cell(int index) {
    return m_parent.cell(index);
}