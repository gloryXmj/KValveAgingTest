#pragma once

#include "src/serial/ISerialTransport.h"

#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>
#include <CSerialPort/SerialPortListener.h>

class CSerialPortAdapter final : public ISerialTransport, private itas109::CSerialPortListener
{
    Q_OBJECT

public:
    explicit CSerialPortAdapter(QObject *parent = nullptr);
    ~CSerialPortAdapter() override;

    QVector<SerialPortDescriptor> availablePorts() const override;
    bool openPort(const SerialPortSettings &settings) override;
    void closePort() override;
    bool isOpen() const override;
    bool writeBytes(const QByteArray &frame) override;

private:
    void onReadEvent(const char *portName, unsigned int readBufferLen) override;
    void publishError(const QString &message);

    itas109::CSerialPort m_serialPort;
};
