#include "src/ui/ValveIndicatorWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

ValveIndicatorWidget::ValveIndicatorWidget(const int channel, const int valveNumber, QWidget *parent)
    : QWidget(parent)
    , m_channel(channel)
    , m_valveNumber(valveNumber)
    , m_pulseTimer(new QTimer(this))
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumSize(sizeHint());
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);

    m_pulseTimer->setSingleShot(true);
    connect(m_pulseTimer, &QTimer::timeout, this, [this]() {
        setPulsing(false);
    });
}

QSize ValveIndicatorWidget::preferredSize(const bool largeTouchMode)
{
    return largeTouchMode ? QSize(64, 82) : QSize(30, 38);
}

int ValveIndicatorWidget::valveNumber() const
{
    return m_valveNumber;
}

bool ValveIndicatorWidget::isPulsing() const
{
    return m_pulsing;
}

void ValveIndicatorWidget::pulse(const int durationMs)
{
    setPulsing(true);
    m_pulseTimer->start(durationMs);
}

void ValveIndicatorWidget::setLargeTouchMode(const bool enabled)
{
    if (m_largeTouchMode == enabled) {
        return;
    }

    m_largeTouchMode = enabled;
    setMinimumSize(sizeHint());
    updateGeometry();
    update();
}

void ValveIndicatorWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF contentRect = rect().adjusted(2.0, 2.0, -2.0, -2.0);
    const qreal textHeight = m_largeTouchMode ? 20.0 : 12.0;
    const qreal circleDiameter = qMin(contentRect.width(), contentRect.height() - textHeight - 2.0);
    const QRectF circleRect(
        contentRect.left() + (contentRect.width() - circleDiameter) / 2.0,
        contentRect.top(),
        circleDiameter,
        circleDiameter
    );
    const QRectF textRect(
        contentRect.left(),
        circleRect.bottom() + 2.0,
        contentRect.width(),
        textHeight
    );

    const QColor fillColor = m_pulsing
        ? QColor(QStringLiteral("#12B76A"))
        : (m_hovered ? QColor(QStringLiteral("#DCE7F8")) : QColor(QStringLiteral("#EEF3F8")));
    const QColor borderColor = m_pulsing
        ? QColor(QStringLiteral("#039855"))
        : (m_hovered ? QColor(QStringLiteral("#8DA8C9")) : QColor(QStringLiteral("#C7D4E2")));
    const QColor textColor = QColor(QStringLiteral("#14324D"));

    painter.setPen(QPen(borderColor, 1.0));
    painter.setBrush(fillColor);
    painter.drawEllipse(circleRect);

    QFont font = painter.font();
    font.setPointSizeF(m_largeTouchMode ? 10.0 : 6.4);
    font.setBold(false);
    painter.setFont(font);
    painter.setPen(textColor);
    painter.drawText(textRect, Qt::AlignCenter, QString::number(m_valveNumber));
}

void ValveIndicatorWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_channel, m_valveNumber);
    }
    QWidget::mousePressEvent(event);
}

void ValveIndicatorWidget::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void ValveIndicatorWidget::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

QSize ValveIndicatorWidget::sizeHint() const
{
    return preferredSize(m_largeTouchMode);
}

void ValveIndicatorWidget::setPulsing(const bool pulsing)
{
    if (m_pulsing == pulsing) {
        return;
    }

    m_pulsing = pulsing;
    update();
}
