#pragma once

#include <QFrame>
#include <QList>

class QLabel;
class ValveGridWidget;

class ChannelCardWidget : public QFrame
{
    Q_OBJECT

public:
    explicit ChannelCardWidget(int channel, QWidget *parent = nullptr);

    int channelNumber() const;
    int testedCount() const;
    int visibleValveCount() const;
    bool isAbnormal() const;
    void pulseValves(const QList<int> &valveNumbers);
    void setVisibleValveCount(int count);
    void setAbnormal(bool abnormal);

signals:
    void valveInvoked(int channel, int valveNumber);

private:
    void refreshVisualState();

    int m_channel = 0;
    int m_testedCount = 0;
    bool m_abnormal = false;
    QLabel *m_badge = nullptr;
    QLabel *m_testedValue = nullptr;
    QLabel *m_lastValue = nullptr;
    ValveGridWidget *m_grid = nullptr;
};
