#include "src/serial/CSerialPortAdapter.h"

#include <QMetaObject>

CSerialPortAdapter::CSerialPortAdapter(QObject *parent)
    : ISerialTransport(parent)
{
}

CSerialPortAdapter::~CSerialPortAdapter()
{
    closePort();
}

QVector<SerialPortDescriptor> CSerialPortAdapter::availablePorts() const
{
    QVector<SerialPortDescriptor> ports;
    const auto nativePorts = itas109::CSerialPortInfo::availablePortInfos();
    ports.reserve(int(nativePorts.size()));

    for (const auto &nativePort : nativePorts) {
        ports.append(SerialPortDescriptor{
            QString::fromLocal8Bit(nativePort.portName),
            QString::fromLocal8Bit(nativePort.description),
            QString::fromLocal8Bit(nativePort.hardwareId),
        });
    }

    return ports;
}

bool CSerialPortAdapter::openPort(const SerialPortSettings &settings)
{
    closePort();

    const QByteArray portName = settings.portName.toLocal8Bit();
    m_serialPort.init(
        portName.constData(),
        settings.baudRate,
        static_cast<itas109::Parity>(settings.parity),
        static_cast<itas109::DataBits>(settings.dataBits),
        static_cast<itas109::StopBits>(settings.stopBits),
        static_cast<itas109::FlowControl>(settings.flowControl),
        4096
    );
    m_serialPort.setReadIntervalTimeout(0);
    m_serialPort.setMinByteReadNotify(1);
    m_serialPort.connectReadEvent(this);

    if (!m_serialPort.open()) {
        publishError(QStringLiteral("打开串口失败：%1").arg(QString::fromLocal8Bit(m_serialPort.getLastErrorMsg())));
        return false;
    }

    emit connectionChanged(true);
    return true;
}

void CSerialPortAdapter::closePort()
{
    if (!m_serialPort.isOpen()) {
        return;
    }

    m_serialPort.disconnectReadEvent();
    m_serialPort.close();
    emit connectionChanged(false);
}

bool CSerialPortAdapter::isOpen() const
{
    return const_cast<itas109::CSerialPort &>(m_serialPort).isOpen();
}

bool CSerialPortAdapter::writeBytes(const QByteArray &frame)
{
    if (!m_serialPort.isOpen()) {
        publishError(QStringLiteral("串口尚未打开。"));
        return false;
    }

    const int written = m_serialPort.writeData(frame.constData(), frame.size());
    if (written != frame.size()) {
        publishError(QStringLiteral("写入串口失败：%1").arg(QString::fromLocal8Bit(m_serialPort.getLastErrorMsg())));
        return false;
    }

    return true;
}

void CSerialPortAdapter::onReadEvent(const char *, const unsigned int readBufferLen)
{
    if (readBufferLen == 0) {
        return;
    }

    QByteArray payload(int(readBufferLen), Qt::Uninitialized);
    const int readLength = m_serialPort.readData(payload.data(), payload.size());
    if (readLength <= 0) {
        publishError(QStringLiteral("读取串口失败：%1").arg(QString::fromLocal8Bit(m_serialPort.getLastErrorMsg())));
        return;
    }

    payload.truncate(readLength);
    QMetaObject::invokeMethod(this, [this, payload]() {
        emit rawDataReceived(payload);
    }, Qt::QueuedConnection);
}

void CSerialPortAdapter::publishError(const QString &message)
{
    QMetaObject::invokeMethod(this, [this, message]() {
        emit errorOccurred(message);
    }, Qt::QueuedConnection);
}
