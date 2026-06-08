#include "src/serial/SerialPortService.h"

SerialPortService::SerialPortService(ISerialTransport *transport, QObject *parent)
    : QObject(parent)
    , m_transport(transport)
{
    if (m_transport != nullptr) {
        connect(m_transport, &ISerialTransport::rawDataReceived, this, &SerialPortService::handleRawDataReceived);
        connect(m_transport, &ISerialTransport::errorOccurred, this, &SerialPortService::transportError);
        connect(m_transport, &ISerialTransport::connectionChanged, this, [this](const bool connected) {
            if (!connected) {
                m_parser.reset();
            }
            emit connectionChanged(connected);
        });
    }
}

QVector<SerialPortDescriptor> SerialPortService::availablePorts() const
{
    return m_transport != nullptr ? m_transport->availablePorts() : QVector<SerialPortDescriptor> {};
}

bool SerialPortService::openPort(const SerialPortSettings &settings)
{
    m_parser.reset();
    return m_transport != nullptr && m_transport->openPort(settings);
}

void SerialPortService::closePort()
{
    if (m_transport == nullptr) {
        return;
    }

    m_transport->closePort();
    m_parser.reset();
}

bool SerialPortService::isOpen() const
{
    return m_transport != nullptr && m_transport->isOpen();
}

bool SerialPortService::writeFrame(const QByteArray &frame)
{
    return m_transport != nullptr && m_transport->writeBytes(frame);
}

void SerialPortService::handleRawDataReceived(const QByteArray &data)
{
    emit rawDataReceived(data);

    const QList<QByteArray> frames = m_parser.pushBytes(data);
    for (const QByteArray &frame : frames) {
        emit ackFrameReceived(frame);
    }
}
