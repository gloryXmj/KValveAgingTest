#pragma once

#include "src/serial/SerialPortTypes.h"

#include <QByteArray>
#include <QObject>
#include <QVector>

class ISerialTransport : public QObject
{
    Q_OBJECT

public:
    explicit ISerialTransport(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~ISerialTransport() override = default;

    virtual QVector<SerialPortDescriptor> availablePorts() const = 0;
    virtual bool openPort(const SerialPortSettings &settings) = 0;
    virtual void closePort() = 0;
    virtual bool isOpen() const = 0;
    virtual bool writeBytes(const QByteArray &frame) = 0;

signals:
    void rawDataReceived(const QByteArray &data);
    void errorOccurred(const QString &message);
    void connectionChanged(bool connected);
};
