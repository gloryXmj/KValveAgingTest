#pragma once

#include "src/serial/AckFrameStreamParser.h"
#include "src/serial/ISerialTransport.h"

#include <QObject>

class SerialPortService : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortService(ISerialTransport *transport, QObject *parent = nullptr);

    QVector<SerialPortDescriptor> availablePorts() const;
    bool openPort(const SerialPortSettings &settings);
    void closePort();
    bool isOpen() const;
    bool writeFrame(const QByteArray &frame);

signals:
    void rawDataReceived(const QByteArray &data);
    void ackFrameReceived(const QByteArray &frame);
    void transportError(const QString &message);
    void connectionChanged(bool connected);

private slots:
    void handleRawDataReceived(const QByteArray &data);

private:
    ISerialTransport *m_transport = nullptr;
    AckFrameStreamParser m_parser;
};
