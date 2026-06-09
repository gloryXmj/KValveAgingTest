#pragma once

#include "src/core/CommandPacket.h"
#include "src/core/ControlParameters.h"

#include <QFrame>
#include <QVector>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QWidget;

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
    void generalBatchRequested();
    void timingBatchRequested();
    void controlParametersChanged(const ControlParameters &parameters);

private:
    void applyOperationModeSelection(int operationMode);
    void emitParameterCommand(quint8 command, quint16 data16, bool passwordRequired, CommandPurpose purpose = CommandPurpose::ParameterWrite);
    void emitControlParametersChanged();
    void updateOperationModeUi(bool syncCommands);
    void updateOperationModeButtons();
    void rebuildTriggerModeOptions(bool syncCommand);
    void updateValveSwitchButton();
    static int agingFrequencyFromIntervalMs(int intervalMs);
    static int intervalMsFromAgingFrequency(int frequency);
    static quint16 timingMsToRaw(double milliseconds);

    int m_operationModeValue = 0;
    QPushButton *m_testModeButton = nullptr;
    QPushButton *m_agingModeButton = nullptr;
    QPushButton *m_valveSwitch = nullptr;
    QComboBox *m_triggerMode = nullptr;
    QSpinBox *m_blowCount = nullptr;
    QSpinBox *m_blowInterval = nullptr;
    QSpinBox *m_agingFrequency = nullptr;
    QDoubleSpinBox *m_blowTime = nullptr;
    QDoubleSpinBox *m_chargeTime = nullptr;
    QDoubleSpinBox *m_stopChargeTime = nullptr;
    QDoubleSpinBox *m_rechargeTime = nullptr;
    QSpinBox *m_channelCount = nullptr;
    QComboBox *m_independentChannelEnable = nullptr;
    QWidget *m_blowCountRow = nullptr;
    QWidget *m_blowIntervalRow = nullptr;
    QWidget *m_agingFrequencyRow = nullptr;
};
