#include "src/ui/PasswordDialog.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QStringList>
#include <QVBoxLayout>

namespace
{
QPushButton *createKeyboardButton(
    const QString &text,
    QWidget *parent,
    const char *propertyName,
    const bool expanding = false)
{
    auto *button = new QPushButton(text, parent);
    button->setProperty(propertyName, true);
    button->setMinimumHeight(52);
    button->setSizePolicy(expanding ? QSizePolicy::Expanding : QSizePolicy::Preferred, QSizePolicy::Fixed);
    return button;
}

QWidget *createKeyboardRow(const QList<QWidget *> &widgets, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    for (auto *widget : widgets) {
        layout->addWidget(widget);
    }

    return row;
}
}

PasswordDialog::PasswordDialog(const QString &prompt, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("确认受保护参数"));
    setModal(true);
    resize(720, 560);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("passwordCard"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(12);

    auto *promptLabel = new QLabel(prompt, card);
    promptLabel->setWordWrap(true);
    cardLayout->addWidget(promptLabel);

    m_passwordEdit = new QLineEdit(card);
    m_passwordEdit->setObjectName(QStringLiteral("passwordEdit"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    cardLayout->addWidget(m_passwordEdit);

    auto *keyboard = new QWidget(card);
    auto *keyboardLayout = new QVBoxLayout(keyboard);
    keyboardLayout->setContentsMargins(0, 0, 0, 0);
    keyboardLayout->setSpacing(8);

    const QStringList digitKeys{
        QStringLiteral("1"),
        QStringLiteral("2"),
        QStringLiteral("3"),
        QStringLiteral("4"),
        QStringLiteral("5"),
        QStringLiteral("6"),
        QStringLiteral("7"),
        QStringLiteral("8"),
        QStringLiteral("9"),
        QStringLiteral("0"),
    };

    QList<QWidget *> digitButtons;
    digitButtons.reserve(digitKeys.size());
    for (const QString &digit : digitKeys) {
        auto *button = createKeyboardButton(digit, keyboard, "keyboardKey", true);
        connect(button, &QPushButton::clicked, this, [this, digit]() {
            appendCharacter(digit, false);
        });
        digitButtons.append(button);
    }
    keyboardLayout->addWidget(createKeyboardRow(digitButtons, keyboard));

    const QStringList letterRow1{
        QStringLiteral("q"),
        QStringLiteral("w"),
        QStringLiteral("e"),
        QStringLiteral("r"),
        QStringLiteral("t"),
        QStringLiteral("y"),
        QStringLiteral("u"),
        QStringLiteral("i"),
        QStringLiteral("o"),
        QStringLiteral("p"),
    };

    QList<QWidget *> row1Buttons;
    row1Buttons.reserve(letterRow1.size());
    for (const QString &letter : letterRow1) {
        auto *button = createKeyboardButton(letter, keyboard, "keyboardKey", true);
        button->setProperty("baseLetter", letter);
        m_letterButtons.append(button);
        connect(button, &QPushButton::clicked, this, [this, letter]() {
            appendCharacter(letter, true);
        });
        row1Buttons.append(button);
    }
    keyboardLayout->addWidget(createKeyboardRow(row1Buttons, keyboard));

    const QStringList letterRow2{
        QStringLiteral("a"),
        QStringLiteral("s"),
        QStringLiteral("d"),
        QStringLiteral("f"),
        QStringLiteral("g"),
        QStringLiteral("h"),
        QStringLiteral("j"),
        QStringLiteral("k"),
        QStringLiteral("l"),
        QStringLiteral("@"),
    };

    QList<QWidget *> row2Buttons;
    row2Buttons.reserve(letterRow2.size());
    for (const QString &key : letterRow2) {
        const bool letterKey = key != QStringLiteral("@");
        auto *button = createKeyboardButton(key, keyboard, "keyboardKey", true);
        if (letterKey) {
            button->setProperty("baseLetter", key);
            m_letterButtons.append(button);
        }
        connect(button, &QPushButton::clicked, this, [this, key, letterKey]() {
            appendCharacter(key, letterKey);
        });
        row2Buttons.append(button);
    }
    keyboardLayout->addWidget(createKeyboardRow(row2Buttons, keyboard));

    QList<QWidget *> row3Buttons;
    m_shiftButton = createKeyboardButton(QStringLiteral("Shift"), keyboard, "keyboardAction", true);
    connect(m_shiftButton, &QPushButton::clicked, this, &PasswordDialog::toggleShift);
    row3Buttons.append(m_shiftButton);

    const QStringList letterRow3{
        QStringLiteral("z"),
        QStringLiteral("x"),
        QStringLiteral("c"),
        QStringLiteral("v"),
        QStringLiteral("b"),
        QStringLiteral("n"),
        QStringLiteral("m"),
    };
    for (const QString &letter : letterRow3) {
        auto *button = createKeyboardButton(letter, keyboard, "keyboardKey", true);
        button->setProperty("baseLetter", letter);
        m_letterButtons.append(button);
        connect(button, &QPushButton::clicked, this, [this, letter]() {
            appendCharacter(letter, true);
        });
        row3Buttons.append(button);
    }

    auto *underscoreButton = createKeyboardButton(QStringLiteral("_"), keyboard, "keyboardKey", true);
    connect(underscoreButton, &QPushButton::clicked, this, [this]() {
        appendCharacter(QStringLiteral("_"), false);
    });
    row3Buttons.append(underscoreButton);

    auto *backspaceButton = createKeyboardButton(QStringLiteral("退格"), keyboard, "keyboardAction", true);
    connect(backspaceButton, &QPushButton::clicked, this, &PasswordDialog::backspace);
    row3Buttons.append(backspaceButton);
    keyboardLayout->addWidget(createKeyboardRow(row3Buttons, keyboard));

    QList<QWidget *> actionButtons;
    auto *clearButton = createKeyboardButton(QStringLiteral("清空"), keyboard, "keyboardAction", true);
    connect(clearButton, &QPushButton::clicked, this, &PasswordDialog::clearPassword);
    actionButtons.append(clearButton);

    auto *cancelButton = createKeyboardButton(QStringLiteral("取消"), keyboard, "keyboardAction", true);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    actionButtons.append(cancelButton);

    auto *confirmButton = createKeyboardButton(QStringLiteral("确认"), keyboard, "keyboardConfirm", true);
    connect(confirmButton, &QPushButton::clicked, this, &QDialog::accept);
    actionButtons.append(confirmButton);
    keyboardLayout->addWidget(createKeyboardRow(actionButtons, keyboard));

    cardLayout->addWidget(keyboard);
    layout->addWidget(card);

    refreshShiftState();
    m_passwordEdit->setFocus();
}

QString PasswordDialog::password() const
{
    return m_passwordEdit->text();
}

void PasswordDialog::setPasswordText(const QString &text)
{
    m_passwordEdit->setText(text);
}

void PasswordDialog::appendCharacter(const QString &character, const bool letterKey)
{
    if (m_passwordEdit == nullptr) {
        return;
    }

    const QString textToInsert = letterKey && m_shiftEnabled ? character.toUpper() : character;
    m_passwordEdit->insert(textToInsert);

    if (letterKey && m_shiftEnabled) {
        m_shiftEnabled = false;
        refreshShiftState();
    }
}

void PasswordDialog::backspace()
{
    if (m_passwordEdit != nullptr) {
        m_passwordEdit->backspace();
    }
}

void PasswordDialog::clearPassword()
{
    if (m_passwordEdit != nullptr) {
        m_passwordEdit->clear();
    }
}

void PasswordDialog::toggleShift()
{
    m_shiftEnabled = !m_shiftEnabled;
    refreshShiftState();
}

void PasswordDialog::refreshShiftState()
{
    if (m_shiftButton != nullptr) {
        m_shiftButton->setProperty("shiftActive", m_shiftEnabled);
        m_shiftButton->style()->unpolish(m_shiftButton);
        m_shiftButton->style()->polish(m_shiftButton);
        m_shiftButton->update();
    }

    for (auto *button : m_letterButtons) {
        if (button == nullptr) {
            continue;
        }

        const QString baseLetter = button->property("baseLetter").toString();
        button->setText(m_shiftEnabled ? baseLetter.toUpper() : baseLetter.toLower());
    }
}
