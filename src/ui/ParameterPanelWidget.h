#pragma once

#include "src/core/CommandPacket.h"

#include <QFrame>

class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QSpinBox;

class ParameterPanelWidget : public QFrame
{
    Q_OBJECT

public:
    explicit ParameterPanelWidget(QWidget *parent = nullptr);

signals:
    void commandRequested(const CommandPacket &packet, bool passwordRequired);

private:
    void emitParameterCommand(quint8 command, quint16 data16, bool passwordRequired, CommandPurpose purpose = CommandPurpose::ParameterWrite);
    static quint16 timingMsToRaw(double milliseconds);

    QComboBox *m_valveSwitch = nullptr;
    QComboBox *m_triggerMode = nullptr;
    QSpinBox *m_blowCount = nullptr;
    QSpinBox *m_blowInterval = nullptr;
    QDoubleSpinBox *m_blowTime = nullptr;
    QDoubleSpinBox *m_chargeTime = nullptr;
    QDoubleSpinBox *m_stopChargeTime = nullptr;
    QDoubleSpinBox *m_rechargeTime = nullptr;
    QSpinBox *m_channelCount = nullptr;
};
