#pragma once

#include <QWidget>

class QAbstractSpinBox;

class TouchSpinBoxWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TouchSpinBoxWidget(
        QAbstractSpinBox *spinBox,
        int fieldWidth,
        const QString &dialogTitle = QString(),
        QWidget *parent = nullptr);

    QAbstractSpinBox *spinBox() const;
    static void prepareSpinBox(QAbstractSpinBox *spinBox, int fieldWidth);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void openNumericInputDialog();
    QString currentValueText() const;

    QAbstractSpinBox *m_spinBox = nullptr;
    QString m_dialogTitle;
    bool m_dialogOpen = false;
};
