#pragma once

#include <QDialog>
#include <QVector>

class QLineEdit;
class QPushButton;

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(const QString &prompt, QWidget *parent = nullptr);

    QString password() const;
    void setPasswordText(const QString &text);

private:
    void appendCharacter(const QString &character, bool letterKey);
    void backspace();
    void clearPassword();
    void toggleShift();
    void refreshShiftState();

    QLineEdit *m_passwordEdit = nullptr;
    QPushButton *m_shiftButton = nullptr;
    QVector<QPushButton *> m_letterButtons;
    bool m_shiftEnabled = false;
};
