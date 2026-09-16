#pragma once

#include "src/core/ControlParameters.h"
#include "src/core/CommandPacket.h"
#include "src/serial/SerialPortTypes.h"

#include <QMainWindow>
#include <QHash>
#include <QSet>
#include <QVector>

class AppSettings;
class ChannelCardWidget;
class LogPanelWidget;
class ParameterPanelWidget;
class QComboBox;
class QGridLayout;
class QLabel;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QTimer;
class SerialWorkerRuntime;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SerialWorkerRuntime *runtime, AppSettings *settings, QWidget *parent = nullptr);

    int channelCardCount() const;

signals:
    void requestAvailablePorts();
    void requestOpenPort(const SerialPortSettings &settings);
    void requestClosePort();
    void requestEnqueueCommand(const CommandPacket &packet, const QString &password);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void refreshPorts();
    void applySavedSettings();
    void enqueueCommands(const QVector<CommandPacket> &packets, const QString &password = QString());
    void applyVisibleValveCount(int count);
    void updateChannelCardLayout(int channelCount);
    void updateConnectionBadge(bool connected);
    void updateChannelAlarmDisplay();
    void handlePortsReady(const QVector<SerialPortDescriptor> &ports);
    void handleCommandRequest(const CommandPacket &packet, bool passwordRequired);
    void handleTimingBatchRequest();
    void handleValveInvoked(int channel, int valveNumber);
    void handleSingleValveCycleToggled(bool running);
    void handleTriggerModeChanged(int triggerMode);
    void startSingleValveCycleTests();
    void startSingleValveCycleTest(int channel);
    void stopSingleValveCycleTest(int channel, bool sendOff = true);
    void stopAllSingleValveCycleTests();
    void handleSingleValveCycleTimeout(int channel);
    void handleCommandSent(const CommandPacket &packet);
    void enqueueSingleValveCycleOpenCommand(int channel, const CommandPacket &packet);
    void setContinuousValve(int channel, int valveNumber);
    void clearContinuousValve(int channel);
    ChannelCardWidget *channelCard(int channel) const;
    bool requestProtectedPassword(
        const QString &prompt,
        const QString &successMessage,
        const QString &cancelMessage,
        QString *password);
    SerialPortSettings currentSerialSettings() const;

    SerialWorkerRuntime *m_runtime = nullptr;
    AppSettings *m_settings = nullptr;
    SerialPortSettings m_savedSerialSettings;
    SerialPortSettings m_lastConnectedSerialSettings;
    ControlParameters m_savedControlParameters;
    int m_savedVisibleValveCount = 0;
    bool m_connected = false;
    bool m_hasCommunicationFault = true;
    bool m_autoConnectPending = false;
    bool m_autoApplyGeneralParametersPending = false;
    QSet<int> m_abnormalChannels;
    struct SingleValveCycleState
    {
        int currentValve = 0;
        int nextValve = 1;
        quint64 pendingOpenRequestId = 0;
    };

    QHash<int, QTimer *> m_singleValveCycleTimers;
    QHash<int, SingleValveCycleState> m_singleValveCycleStates;
    quint64 m_nextCycleRequestId = 1;
    int m_manualContinuousChannel = 0;
    int m_manualContinuousValve = 0;

    QComboBox *m_portCombo = nullptr;
    QComboBox *m_baudCombo = nullptr;
    QSpinBox *m_visibleValveCountSpin = nullptr;
    QPushButton *m_connectButton = nullptr;
    QLabel *m_connectionBadge = nullptr;
    QLabel *m_firmwareValue = nullptr;
    ParameterPanelWidget *m_parameterPanel = nullptr;
    LogPanelWidget *m_logPanel = nullptr;
    QScrollArea *m_parameterScroll = nullptr;
    QScrollArea *m_cardsScroll = nullptr;
    QWidget *m_cardsPage = nullptr;
    QGridLayout *m_cardsLayout = nullptr;
    QVector<ChannelCardWidget *> m_channelCards;
};
