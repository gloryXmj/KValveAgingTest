#pragma once

#include "src/core/CommandPacket.h"
#include "src/core/ProtocolCodec.h"
#include "src/serial/SerialPortTypes.h"

#include <QObject>
#include <QQueue>
#include <QVector>

#include <optional>

class SerialPortService;
class QTimer;

class ValveTestController : public QObject
{
    Q_OBJECT

public:
    explicit ValveTestController(SerialPortService *serialService, QObject *parent = nullptr);

    QVector<SerialPortDescriptor> availablePorts() const;
    bool openPort(const SerialPortSettings &settings);
    void closePort();
    bool isConnected() const;
    bool isBusy() const;

    void setAckTimeoutMs(int timeoutMs);
    void enqueueCommand(const CommandPacket &packet, const QString &password = QString());

signals:
    void commandRejected(const QString &reason);
    void logGenerated(const QString &direction, const QString &hex, const QString &description);
    void valveActionConfirmed(int channel, const QList<int> &valveNumbers);
    void channelAlarmUpdated(const QList<int> &abnormalChannels);
    void versionReceived(const QString &versionText);
    void commandTimedOut(int command, int data16);
    void busyChanged(bool busy);
    void connectionChanged(bool connected);

private slots:
    void handleAckFrame(const QByteArray &frame);
    void handleTransportError(const QString &message);
    void handleAckTimeout();

private:
    struct PendingCommand
    {
        CommandPacket packet;
        QByteArray frame;
    };

    void trySendNext();
    void completeInFlight();
    bool ackMatchesInFlight(const DecodedFrame &frame) const;
    QString describeOutgoing(const CommandPacket &packet) const;
    QString describeAck(const DecodedFrame &frame) const;
    QString formatVersion(quint16 data16) const;

    SerialPortService *m_serialService = nullptr;
    QQueue<PendingCommand> m_queue;
    std::optional<PendingCommand> m_inFlight;
    QTimer *m_ackTimer = nullptr;
    int m_ackTimeoutMs = 500;
};
