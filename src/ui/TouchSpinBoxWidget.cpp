#include "src/ui/TouchSpinBoxWidget.h"

#include "src/ui/NumericInputDialog.h"

#include <QAbstractSpinBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSpinBox>

namespace
{
QString trimTrailingZeros(QString text)
{
    while (text.contains(QChar('.')) && text.endsWith(QChar('0'))) {
        text.chop(1);
    }
    if (text.endsWith(QChar('.'))) {
        text.chop(1);
    }
    return text;
}
}

TouchSpinBoxWidget::TouchSpinBoxWidget(
    QAbstractSpinBox *spinBox,
    const int fieldWidth,
    const QString &dialogTitle,
    QWidget *parent)
    : QWidget(parent)
    , m_spinBox(spinBox)
    , m_dialogTitle(dialogTitle)
{
    setProperty("touchSpinWidget", true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (m_spinBox != nullptr) {
        m_spinBox->setParent(this);
        prepareSpinBox(m_spinBox, fieldWidth);
        m_spinBox->installEventFilter(this);
        if (auto *lineEdit = m_spinBox->findChild<QLineEdit *>(); lineEdit != nullptr) {
            lineEdit->setCursor(Qt::PointingHandCursor);
            lineEdit->installEventFilter(this);
        }
        layout->addWidget(m_spinBox, 1);
    }
}

QAbstractSpinBox *TouchSpinBoxWidget::spinBox() const
{
    return m_spinBox;
}

void TouchSpinBoxWidget::prepareSpinBox(QAbstractSpinBox *spinBox, const int fieldWidth)
{
    if (spinBox == nullptr) {
        return;
    }

    spinBox->setProperty("touchSpinField", true);
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinBox->setAlignment(Qt::AlignCenter);
    spinBox->setAccelerated(true);
    spinBox->setKeyboardTracking(false);
    spinBox->setReadOnly(true);
    spinBox->setCursor(Qt::PointingHandCursor);
    spinBox->setMinimumHeight(44);
    spinBox->setMinimumWidth(fieldWidth);
}

bool TouchSpinBoxWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (m_spinBox != nullptr
        && (watched == m_spinBox || watched->parent() == m_spinBox)
        && event->type() == QEvent::MouseButtonPress) {
        openNumericInputDialog();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void TouchSpinBoxWidget::openNumericInputDialog()
{
    if (m_spinBox == nullptr || m_dialogOpen) {
        return;
    }

    m_dialogOpen = true;
    const bool allowDecimal = qobject_cast<QDoubleSpinBox *>(m_spinBox) != nullptr;
    NumericInputDialog dialog(
        m_dialogTitle.isEmpty() ? QStringLiteral("输入数值") : m_dialogTitle,
        allowDecimal,
        this
    );
    dialog.setValueText(currentValueText());

    if (dialog.exec() == QDialog::Accepted) {
        const QString valueText = dialog.valueText().trimmed();
        if (!valueText.isEmpty()) {
            if (auto *intSpinBox = qobject_cast<QSpinBox *>(m_spinBox); intSpinBox != nullptr) {
                bool ok = false;
                const int value = valueText.toInt(&ok);
                if (ok) {
                    intSpinBox->setValue(value);
                }
            } else if (auto *doubleSpinBox = qobject_cast<QDoubleSpinBox *>(m_spinBox); doubleSpinBox != nullptr) {
                bool ok = false;
                const double value = valueText.toDouble(&ok);
                if (ok) {
                    doubleSpinBox->setValue(value);
                }
            }
        }
    }

    m_dialogOpen = false;
}

QString TouchSpinBoxWidget::currentValueText() const
{
    if (m_spinBox == nullptr) {
        return QString();
    }

    if (const auto *intSpinBox = qobject_cast<const QSpinBox *>(m_spinBox); intSpinBox != nullptr) {
        return QString::number(intSpinBox->value());
    }

    if (const auto *doubleSpinBox = qobject_cast<const QDoubleSpinBox *>(m_spinBox); doubleSpinBox != nullptr) {
        return trimTrailingZeros(QString::number(doubleSpinBox->value(), 'f', doubleSpinBox->decimals()));
    }

    return QString();
}
