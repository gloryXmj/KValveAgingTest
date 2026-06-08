#pragma once

#include "src/core/ControlParameters.h"
#include "src/core/CommandPacket.h"
#include "src/serial/SerialPortTypes.h"

#include <QMainWindow>
#include <QSet>
#include <QVector>

class AppSettings;
class ChannelCardWidget;
class LogPanelWidget;
class ParameterPanelWidget;
class QComboBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QSpinBox;
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
    void updateConnectionBadge(bool connected);
    void updateChannelAlarmDisplay();
    void handlePortsReady(const QVector<SerialPortDescriptor> &ports);
    void handleCommandRequest(const CommandPacket &packet, bool passwordRequired);
    void handleTimingBatchRequest();
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

    QComboBox *m_portCombo = nullptr;
    QComboBox *m_baudCombo = nullptr;
    QSpinBox *m_visibleValveCountSpin = nullptr;
    QPushButton *m_connectButton = nullptr;
    QLabel *m_connectionBadge = nullptr;
    QLabel *m_firmwareValue = nullptr;
    ParameterPanelWidget *m_parameterPanel = nullptr;
    LogPanelWidget *m_logPanel = nullptr;
    QScrollArea *m_cardsScroll = nullptr;
    QWidget *m_cardsPage = nullptr;
    QVector<ChannelCardWidget *> m_channelCards;
};
