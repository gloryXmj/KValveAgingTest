#pragma once

#include <QDialog>

class QLineEdit;

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(const QString &prompt, QWidget *parent = nullptr);

    QString password() const;
    void setPasswordText(const QString &text);

private:
    QLineEdit *m_passwordEdit = nullptr;
};
