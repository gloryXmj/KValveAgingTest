#include "src/ui/PasswordDialog.h"

#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

PasswordDialog::PasswordDialog(const QString &prompt, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("确认受保护参数"));
    setModal(true);
    resize(380, 180);

    auto *layout = new QVBoxLayout(this);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("passwordCard"));
    auto *cardLayout = new QVBoxLayout(card);

    auto *promptLabel = new QLabel(prompt, card);
    promptLabel->setWordWrap(true);
    cardLayout->addWidget(promptLabel);

    m_passwordEdit = new QLineEdit(card);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    cardLayout->addWidget(m_passwordEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, card);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    cardLayout->addWidget(buttons);

    layout->addWidget(card);
}

QString PasswordDialog::password() const
{
    return m_passwordEdit->text();
}

void PasswordDialog::setPasswordText(const QString &text)
{
    m_passwordEdit->setText(text);
}
