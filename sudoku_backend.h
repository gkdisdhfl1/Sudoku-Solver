#ifndef SUDOKU_BACKEND_H
#define SUDOKU_BACKEND_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

class SudokuBackend : public QObject
{
    Q_OBJECT
    // QML에서 접근할 때 사용할 1차원 형태의 프로퍼티
    Q_PROPERTY(QList<int> board READ board NOTIFY boardChanged)
    Q_PROPERTY(QList<int> errorCells READ errorCells NOTIFY errorCellsChanged)
    QML_ELEMENT // QML에서 직접 사용할 수 있게 등록
public:
    explicit SudokuBackend(QObject *parent = nullptr);

    QList<int> board() const;
    QList<int> errorCells() const;

    Q_INVOKABLE void setCell(int index, int value);
    // Q_INVOKABLE int getCell(int index) const;
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool isValidBoard() const;

    // Algorithm
    Q_INVOKABLE bool solveBacktracking();

signals:
    void boardChanged();
    void errorCellsChanged();

private:
    QVector<QVector<int>> m_board; // 0~80, 0 means empty
    QList<int> m_errorCells;

    bool isValid(const QVector<QVector<int>> &board, int r, int c, int num, bool ignoreSelf = false) const;
    bool solveBacktrackingHelper(QVector<QVector<int>> &board);
    void checkErrors(); // 에러 검사 수행 및 리스트 업데이트
};

#endif // SUDOKU_BACKEND_H
