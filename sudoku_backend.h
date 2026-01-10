#ifndef SUDOKU_BACKEND_H
#define SUDOKU_BACKEND_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

class SudokuBackend : public QObject
{
    Q_OBJECT
    // QML에서 접근할 때 사용할 1차원 형태의 프로퍼티
    Q_PROPERTY(QList<int> board READ board NOTIFY boardChanged)
    QML_ELEMENT // QML에서 직접 사용할 수 있게 등록
public:
    explicit SudokuBackend(QObject *parent = nullptr);

    QList<int> board() const;

    Q_INVOKABLE void setCell(int index, int value);
    // Q_INVOKABLE int getCell(int index) const;
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool isValidBoard();

    // Algorithm
    Q_INVOKABLE bool solveBacktracking();

signals:
    void boardChanged();

private:
    QVector<QVector<int>> m_board; // 0~80, 0 means empty


    bool isValid(const QVector<QVector<int>> &board, int r, int c, int num);
    bool solveBacktrackingHelper(QVector<QVector<int>> &board);
};

#endif // SUDOKU_BACKEND_H
