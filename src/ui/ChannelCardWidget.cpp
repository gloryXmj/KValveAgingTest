#include "src/ui/ChannelCardWidget.h"

#include "src/ui/ValveGridWidget.h"

#include <QFont>
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

    m_rootLayout = new QVBoxLayout(this);
    m_rootLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_rootLayout->setContentsMargins(18, 18, 18, 18);
    m_rootLayout->setSpacing(12);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    m_titleLabel = new QLabel(QStringLiteral("通道 %1").arg(m_channel), this);
    m_titleLabel->setObjectName(QStringLiteral("sectionTitle"));

    m_badge = new QLabel(QStringLiteral("正常"), this);
    m_badge->setObjectName(QStringLiteral("channelBadge"));
    m_badge->setProperty("alarm", false);

    m_testedLabel = new QLabel(QStringLiteral("已测阀数"), this);
    m_testedLabel->setObjectName(QStringLiteral("sectionCaption"));

    m_testedValue = new QLabel(QStringLiteral("0"), this);
    m_testedValue->setObjectName(QStringLiteral("sectionTitle"));

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_badge);
    headerLayout->addStretch();
    headerLayout->addWidget(m_testedLabel);
    headerLayout->addWidget(m_testedValue);
    m_rootLayout->addLayout(headerLayout);

    auto *lastLayout = new QHBoxLayout();
    lastLayout->setContentsMargins(0, 0, 0, 0);
    lastLayout->setSpacing(8);

    m_lastLabel = new QLabel(QStringLiteral("最近回包"), this);
    m_lastLabel->setObjectName(QStringLiteral("sectionCaption"));

    m_lastValue = new QLabel(QStringLiteral("--"), this);
    m_lastValue->setObjectName(QStringLiteral("sectionCaption"));

    lastLayout->addWidget(m_lastLabel);
    lastLayout->addWidget(m_lastValue, 1);
    m_rootLayout->addLayout(lastLayout);

    m_gridCaption = new QLabel(QStringLiteral("阀位指示"), this);
    m_gridCaption->setObjectName(QStringLiteral("sectionCaption"));
    m_rootLayout->addWidget(m_gridCaption);

    m_grid = new ValveGridWidget(m_channel, this);
    connect(m_grid, &ValveGridWidget::valveInvoked, this, &ChannelCardWidget::valveInvoked);
    m_rootLayout->addWidget(m_grid);
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

void ChannelCardWidget::setLargeTouchMode(const bool enabled)
{
    if (m_largeTouchMode == enabled) {
        return;
    }

    m_largeTouchMode = enabled;
    setMinimumHeight(enabled ? 500 : 420);

    if (m_rootLayout != nullptr) {
        m_rootLayout->setContentsMargins(enabled ? 10 : 18, enabled ? 10 : 18, enabled ? 10 : 18, enabled ? 10 : 18);
        m_rootLayout->setSpacing(enabled ? 6 : 12);
    }

    if (m_testedLabel != nullptr) {
        m_testedLabel->setText(enabled ? QStringLiteral("已测") : QStringLiteral("已测阀数"));
        m_testedLabel->setStyleSheet(enabled ? QStringLiteral("font-size: 11px;") : QString());
    }
    if (m_lastLabel != nullptr) {
        m_lastLabel->setText(enabled ? QStringLiteral("回包") : QStringLiteral("最近回包"));
        m_lastLabel->setStyleSheet(enabled ? QStringLiteral("font-size: 11px;") : QString());
    }
    if (m_lastValue != nullptr) {
        m_lastValue->setStyleSheet(enabled ? QStringLiteral("font-size: 11px; color: #44566C;") : QString());
    }
    if (m_testedValue != nullptr) {
        QFont font = m_testedValue->font();
        font.setPointSizeF(enabled ? 13.0 : 15.0);
        font.setBold(true);
        m_testedValue->setFont(font);
    }
    if (m_gridCaption != nullptr) {
        m_gridCaption->setVisible(!enabled);
    }
    if (m_grid != nullptr) {
        m_grid->setLargeTouchMode(enabled);
    }
    if (layout() != nullptr) {
        layout()->activate();
    }
    adjustSize();
    updateGeometry();
}

void ChannelCardWidget::refreshVisualState()
{
    setProperty("alarm", m_abnormal);
    m_badge->setProperty("alarm", m_abnormal);
    m_badge->setText(m_abnormal ? QStringLiteral("报警") : QStringLiteral("正常"));

    style()->unpolish(this);
    style()->polish(this);
    m_badge->style()->unpolish(m_badge);
    m_badge->style()->polish(m_badge);
    update();
}
