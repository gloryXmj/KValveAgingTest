#include "src/ui/NumericInputDialog.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QSizePolicy>
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

NumericInputDialog::NumericInputDialog(const QString &title, const bool allowDecimal, QWidget *parent)
    : QDialog(parent)
    , m_allowDecimal(allowDecimal)
{
    setWindowTitle(title.isEmpty() ? QStringLiteral("输入数值") : title);
    setModal(true);
    resize(520, 460);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("passwordCard"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(12);

    auto *promptLabel = new QLabel(QStringLiteral("请输入新的参数值"), card);
    promptLabel->setWordWrap(true);
    cardLayout->addWidget(promptLabel);

    m_valueEdit = new QLineEdit(card);
    m_valueEdit->setObjectName(QStringLiteral("numericEdit"));
    m_valueEdit->setReadOnly(true);
    m_valueEdit->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_valueEdit);

    auto *keyboard = new QWidget(card);
    auto *keyboardLayout = new QVBoxLayout(keyboard);
    keyboardLayout->setContentsMargins(0, 0, 0, 0);
    keyboardLayout->setSpacing(8);

    const QStringList row1Keys{
        QStringLiteral("1"),
        QStringLiteral("2"),
        QStringLiteral("3"),
    };
    QList<QWidget *> row1Buttons;
    for (const QString &key : row1Keys) {
        auto *button = createKeyboardButton(key, keyboard, "keyboardKey", true);
        connect(button, &QPushButton::clicked, this, [this, key]() {
            appendCharacter(key);
        });
        row1Buttons.append(button);
    }
    keyboardLayout->addWidget(createKeyboardRow(row1Buttons, keyboard));

    const QStringList row2Keys{
        QStringLiteral("4"),
        QStringLiteral("5"),
        QStringLiteral("6"),
    };
    QList<QWidget *> row2Buttons;
    for (const QString &key : row2Keys) {
        auto *button = createKeyboardButton(key, keyboard, "keyboardKey", true);
        connect(button, &QPushButton::clicked, this, [this, key]() {
            appendCharacter(key);
        });
        row2Buttons.append(button);
    }
    keyboardLayout->addWidget(createKeyboardRow(row2Buttons, keyboard));

    const QStringList row3Keys{
        QStringLiteral("7"),
        QStringLiteral("8"),
        QStringLiteral("9"),
    };
    QList<QWidget *> row3Buttons;
    for (const QString &key : row3Keys) {
        auto *button = createKeyboardButton(key, keyboard, "keyboardKey", true);
        connect(button, &QPushButton::clicked, this, [this, key]() {
            appendCharacter(key);
        });
        row3Buttons.append(button);
    }
    keyboardLayout->addWidget(createKeyboardRow(row3Buttons, keyboard));

    QList<QWidget *> row4Buttons;
    auto *decimalButton = createKeyboardButton(QStringLiteral("."), keyboard, "keyboardKey", true);
    decimalButton->setEnabled(m_allowDecimal);
    connect(decimalButton, &QPushButton::clicked, this, [this]() {
        appendCharacter(QStringLiteral("."));
    });
    row4Buttons.append(decimalButton);

    auto *zeroButton = createKeyboardButton(QStringLiteral("0"), keyboard, "keyboardKey", true);
    connect(zeroButton, &QPushButton::clicked, this, [this]() {
        appendCharacter(QStringLiteral("0"));
    });
    row4Buttons.append(zeroButton);

    auto *backspaceButton = createKeyboardButton(QStringLiteral("退格"), keyboard, "keyboardAction", true);
    connect(backspaceButton, &QPushButton::clicked, this, &NumericInputDialog::backspace);
    row4Buttons.append(backspaceButton);
    keyboardLayout->addWidget(createKeyboardRow(row4Buttons, keyboard));

    QList<QWidget *> actionButtons;
    auto *clearButton = createKeyboardButton(QStringLiteral("清空"), keyboard, "keyboardAction", true);
    connect(clearButton, &QPushButton::clicked, this, &NumericInputDialog::clearValue);
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
}

QString NumericInputDialog::valueText() const
{
    return m_valueEdit != nullptr ? m_valueEdit->text() : QString();
}

void NumericInputDialog::setValueText(const QString &text)
{
    if (m_valueEdit != nullptr) {
        m_valueEdit->setText(text);
    }
}

void NumericInputDialog::appendCharacter(const QString &character)
{
    if (m_valueEdit == nullptr) {
        return;
    }

    if (character == QStringLiteral(".") && (!m_allowDecimal || m_valueEdit->text().contains(QChar('.')))) {
        return;
    }

    m_valueEdit->insert(character);
}

void NumericInputDialog::backspace()
{
    if (m_valueEdit != nullptr) {
        m_valueEdit->backspace();
    }
}

void NumericInputDialog::clearValue()
{
    if (m_valueEdit != nullptr) {
        m_valueEdit->clear();
    }
}
