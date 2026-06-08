#pragma once

#include <QDialog>

class QLineEdit;

class NumericInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NumericInputDialog(const QString &title, bool allowDecimal, QWidget *parent = nullptr);

    QString valueText() const;
    void setValueText(const QString &text);

private:
    void appendCharacter(const QString &character);
    void backspace();
    void clearValue();

    QLineEdit *m_valueEdit = nullptr;
    bool m_allowDecimal = false;
};
