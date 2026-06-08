#pragma once

#include "src/core/CommandPacket.h"
#include "src/serial/SerialPortTypes.h"

#include <QObject>

class CSerialPortAdapter;
class SerialPortService;
class ValveTestController;

class SerialWorkerRuntime : public QObject
{
    Q_OBJECT

public:
    explicit SerialWorkerRuntime(QObject *parent = nullptr);

public slots:
    void requestAvailablePorts();
    void requestOpenPort(const SerialPortSettings &settings);
    void requestClosePort();
    void requestEnqueueCommand(const CommandPacket &packet, const QString &password);

signals:
    void portsReady(const QVector<SerialPortDescriptor> &ports);
    void commandRejected(const QString &reason);
    void logGenerated(const QString &direction, const QString &hex, const QString &description);
    void valveActionConfirmed(int channel, const QList<int> &valves);
    void channelAlarmUpdated(const QList<int> &channels);
    void versionReceived(const QString &versionText);
    void busyChanged(bool busy);
    void connectionChanged(bool connected);

private:
    CSerialPortAdapter *m_transport = nullptr;
    SerialPortService *m_serialService = nullptr;
    ValveTestController *m_controller = nullptr;
};
