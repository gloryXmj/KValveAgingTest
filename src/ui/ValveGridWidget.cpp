#include "src/ui/ValveGridWidget.h"

#include "src/core/CommandMap.h"
#include "src/ui/ValveIndicatorWidget.h"

#include <QGridLayout>
#include <QResizeEvent>

namespace
{
constexpr int kMaxColumns = 16;
constexpr int kHorizontalSpacing = 6;
constexpr int kVerticalSpacing = 18;
constexpr int kVerticalSpacingLargeTouch = 38;
constexpr int kDefaultLayoutWidth = 560;
}

ValveGridWidget::ValveGridWidget(const int channel, QWidget *parent)
    : QWidget(parent)
    , m_channel(channel)
    , m_visibleValveCount(CommandMap::kValvesPerChannel)
    , m_layout(new QGridLayout(this))
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    setMinimumWidth(0);

    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setHorizontalSpacing(kHorizontalSpacing);
    m_layout->setVerticalSpacing(kVerticalSpacing);

    for (int valveNumber = 1; valveNumber <= CommandMap::kValvesPerChannel; ++valveNumber) {
        auto *indicator = new ValveIndicatorWidget(m_channel, valveNumber, this);
        connect(indicator, &ValveIndicatorWidget::clicked, this, &ValveGridWidget::valveInvoked);
        m_indicators.insert(valveNumber, indicator);
    }

    rebuildGrid();
}

int ValveGridWidget::valveCount() const
{
    return m_visibleValveCount;
}

int ValveGridWidget::visibleValveCount() const
{
    return m_visibleValveCount;
}

bool ValveGridWidget::isValvePulsing(const int valveNumber) const
{
    const auto it = m_indicators.constFind(valveNumber);
    return it != m_indicators.cend() && it.value()->isPulsing();
}

void ValveGridWidget::setVisibleValveCount(const int count)
{
    const int boundedCount = qBound(1, count, CommandMap::kValvesPerChannel);
    if (m_visibleValveCount == boundedCount) {
        return;
    }

    m_visibleValveCount = boundedCount;
    rebuildGrid();
}

void ValveGridWidget::setLargeTouchMode(const bool enabled)
{
    if (m_largeTouchMode == enabled) {
        return;
    }

    m_largeTouchMode = enabled;
    for (auto it = m_indicators.begin(); it != m_indicators.end(); ++it) {
        it.value()->setLargeTouchMode(enabled);
    }
    rebuildGrid();
}

void ValveGridWidget::pulseValves(const QList<int> &valveNumbers)
{
    for (const int valveNumber : valveNumbers) {
        const auto it = m_indicators.find(valveNumber);
        if (it != m_indicators.end()) {
            it.value()->pulse();
        }
    }
}

QSize ValveGridWidget::sizeHint() const
{
    return QSize(width() > 0 ? width() : kDefaultLayoutWidth, height());
}

QSize ValveGridWidget::minimumSizeHint() const
{
    return QSize(0, height());
}

void ValveGridWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    const int nextColumnCount = calculateColumnCount(event->size().width());
    if (nextColumnCount != m_columnCount) {
        rebuildGrid();
    }
}

void ValveGridWidget::updateIndicatorVisibility()
{
    for (auto it = m_indicators.begin(); it != m_indicators.end(); ++it) {
        it.value()->setVisible(it.key() <= m_visibleValveCount);
    }
}

void ValveGridWidget::rebuildGrid()
{
    updateIndicatorVisibility();
    if (m_layout != nullptr) {
        m_layout->setVerticalSpacing(m_largeTouchMode ? kVerticalSpacingLargeTouch : kVerticalSpacing);
    }

    const int availableWidth = width() > 0 ? width() : kDefaultLayoutWidth;
    const int columns = calculateColumnCount(availableWidth);
    m_columnCount = columns;

    for (int valveNumber = 1; valveNumber <= CommandMap::kValvesPerChannel; ++valveNumber) {
        if (auto *indicator = m_indicators.value(valveNumber, nullptr)) {
            m_layout->removeWidget(indicator);
        }
    }

    for (int valveNumber = 1; valveNumber <= m_visibleValveCount; ++valveNumber) {
        auto *indicator = m_indicators.value(valveNumber, nullptr);
        if (indicator == nullptr) {
            continue;
        }

        const int zeroBased = valveNumber - 1;
        const int row = zeroBased / columns;
        const int column = zeroBased % columns;
        m_layout->addWidget(indicator, row, column, Qt::AlignTop | Qt::AlignHCenter);
    }

    const int indicatorHeight = ValveIndicatorWidget::preferredSize(m_largeTouchMode).height();
    const int rowCount = qMax(1, (m_visibleValveCount + columns - 1) / columns);
    const QMargins margins = m_layout->contentsMargins();
    const int contentHeight = margins.top()
        + margins.bottom()
        + (rowCount * indicatorHeight)
        + ((rowCount - 1) * m_layout->verticalSpacing());

    setFixedHeight(contentHeight);
    if (m_layout != nullptr) {
        m_layout->activate();
    }
    adjustSize();
    updateGeometry();
}

int ValveGridWidget::calculateColumnCount(const int availableWidth) const
{
    const int indicatorWidth = ValveIndicatorWidget::preferredSize(m_largeTouchMode).width();
    const int spacing = m_layout != nullptr ? m_layout->horizontalSpacing() : kHorizontalSpacing;
    const int effectiveWidth = qMax(indicatorWidth, availableWidth);
    const int calculated = qMax(1, (effectiveWidth + spacing) / (indicatorWidth + spacing));
    return qMin(qMin(calculated, kMaxColumns), m_visibleValveCount);
}
