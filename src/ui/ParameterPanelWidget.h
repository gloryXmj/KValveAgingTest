#pragma once

#include "src/core/CommandPacket.h"
#include "src/core/ControlParameters.h"

#include <QFrame>
#include <QVector>

class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QSpinBox;

class ParameterPanelWidget : public QFrame
{
    Q_OBJECT

public:
    explicit ParameterPanelWidget(QWidget *parent = nullptr);

    ControlParameters currentControlParameters() const;
    void applyControlParameters(const ControlParameters &parameters);
    QVector<CommandPacket> generalParameterCommands() const;
    QVector<CommandPacket> timingParameterCommands() const;

signals:
    void commandRequested(const CommandPacket &packet, bool passwordRequired);
    void timingBatchRequested();
    void controlParametersChanged(const ControlParameters &parameters);

private:
    void emitParameterCommand(quint8 command, quint16 data16, bool passwordRequired, CommandPurpose purpose = CommandPurpose::ParameterWrite);
    void emitControlParametersChanged();
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
    QComboBox *m_independentChannelEnable = nullptr;
};
