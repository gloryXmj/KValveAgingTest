#include "src/ui/ChannelCardWidget.h"

#include "src/ui/ValveGridWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QStyle>
#include <QVariant>
#include <QVBoxLayout>

ChannelCardWidget::ChannelCardWidget(const int channel, QWidget *parent)
    : QFrame(parent)
    , m_channel(channel)
{
    setObjectName(QStringLiteral("channelCard"));
    setProperty("alarm", false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumWidth(0);
    setMinimumHeight(420);

    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel(QStringLiteral("通道 %1").arg(m_channel), this);
    titleLabel->setObjectName(QStringLiteral("sectionTitle"));

    m_badge = new QLabel(QStringLiteral("正常"), this);
    m_badge->setObjectName(QStringLiteral("channelBadge"));
    m_badge->setProperty("alarm", false);

    auto *testedLabel = new QLabel(QStringLiteral("已测阀数"), this);
    testedLabel->setObjectName(QStringLiteral("sectionCaption"));
    m_testedValue = new QLabel(QStringLiteral("0"), this);
    m_testedValue->setObjectName(QStringLiteral("sectionTitle"));

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(m_badge);
    headerLayout->addStretch();
    headerLayout->addWidget(testedLabel);
    headerLayout->addWidget(m_testedValue);
    layout->addLayout(headerLayout);

    auto *lastLayout = new QHBoxLayout();
    auto *lastLabel = new QLabel(QStringLiteral("最近回包"), this);
    lastLabel->setObjectName(QStringLiteral("sectionCaption"));
    m_lastValue = new QLabel(QStringLiteral("--"), this);
    lastLayout->addWidget(lastLabel);
    lastLayout->addWidget(m_lastValue, 1);
    layout->addLayout(lastLayout);

    auto *gridCaption = new QLabel(QStringLiteral("阀位指示"), this);
    gridCaption->setObjectName(QStringLiteral("sectionCaption"));
    layout->addWidget(gridCaption);

    m_grid = new ValveGridWidget(m_channel, this);
    connect(m_grid, &ValveGridWidget::valveInvoked, this, &ChannelCardWidget::valveInvoked);
    layout->addWidget(m_grid);
}

int ChannelCardWidget::channelNumber() const
{
    return m_channel;
}

int ChannelCardWidget::testedCount() const
{
    return m_testedCount;
}

int ChannelCardWidget::visibleValveCount() const
{
    return m_grid != nullptr ? m_grid->visibleValveCount() : 0;
}

bool ChannelCardWidget::isAbnormal() const
{
    return m_abnormal;
}

void ChannelCardWidget::pulseValves(const QList<int> &valveNumbers)
{
    if (valveNumbers.isEmpty()) {
        return;
    }

    m_grid->pulseValves(valveNumbers);
    m_testedCount += valveNumbers.size();
    m_testedValue->setText(QString::number(m_testedCount));

    QStringList labels;
    labels.reserve(valveNumbers.size());
    for (const int valveNumber : valveNumbers) {
        labels.append(QString::number(valveNumber));
    }
    m_lastValue->setText(labels.join(QStringLiteral(", ")));
}

void ChannelCardWidget::setVisibleValveCount(const int count)
{
    if (m_grid != nullptr) {
        m_grid->setVisibleValveCount(count);
        if (layout() != nullptr) {
            layout()->activate();
        }
        adjustSize();
        updateGeometry();
    }
}

void ChannelCardWidget::setAbnormal(const bool abnormal)
{
    if (m_abnormal == abnormal) {
        return;
    }

    m_abnormal = abnormal;
    refreshVisualState();
}

void ChannelCardWidget::refreshVisualState()
{
    setProperty("alarm", m_abnormal);
    m_badge->setProperty("alarm", m_abnormal);
    m_badge->setText(m_abnormal ? QStringLiteral("告警") : QStringLiteral("正常"));

    style()->unpolish(this);
    style()->polish(this);
    m_badge->style()->unpolish(m_badge);
    m_badge->style()->polish(m_badge);
    update();
}
