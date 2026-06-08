#include "src/controller/ValveTestController.h"

#include "src/core/CommandMap.h"
#include "src/core/ProtocolCodec.h"
#include "src/core/SecurityPolicy.h"
#include "src/core/ValveAddressResolver.h"
#include "src/serial/SerialPortService.h"
#include "src/serial/SerialPortTypes.h"

#include <QStringList>

namespace
{
QString formatTimingMilliseconds(const quint16 rawValue)
{
    return QStringLiteral("%1 ms").arg(QString::number(double(rawValue) * 0.05, 'f', 2));
}
}

ValveTestController::ValveTestController(SerialPortService *serialService, QObject *parent)
    : QObject(parent)
    , m_serialService(serialService)
{
    m_ackTimer.setSingleShot(true);

    connect(&m_ackTimer, &QTimer::timeout, this, &ValveTestController::handleAckTimeout);

    if (m_serialService != nullptr) {
        connect(m_serialService, &SerialPortService::ackFrameReceived, this, &ValveTestController::handleAckFrame);
        connect(m_serialService, &SerialPortService::transportError, this, &ValveTestController::handleTransportError);
        connect(m_serialService, &SerialPortService::connectionChanged, this, &ValveTestController::connectionChanged);
    }
}

QVector<SerialPortDescriptor> ValveTestController::availablePorts() const
{
    return m_serialService != nullptr ? m_serialService->availablePorts() : QVector<SerialPortDescriptor> {};
}

bool ValveTestController::openPort(const SerialPortSettings &settings)
{
    return m_serialService != nullptr && m_serialService->openPort(settings);
}

void ValveTestController::closePort()
{
    if (m_serialService == nullptr) {
        return;
    }

    m_serialService->closePort();
    m_queue.clear();
    if (m_inFlight.has_value()) {
        m_inFlight.reset();
        m_ackTimer.stop();
        emit busyChanged(false);
    }
}

bool ValveTestController::isConnected() const
{
    return m_serialService != nullptr && m_serialService->isOpen();
}

bool ValveTestController::isBusy() const
{
    return m_inFlight.has_value();
}

void ValveTestController::setAckTimeoutMs(const int timeoutMs)
{
    m_ackTimeoutMs = qMax(1, timeoutMs);
}

void ValveTestController::enqueueCommand(const CommandPacket &packet, const QString &password)
{
    if (SecurityPolicy::requiresPassword(packet.command) && !SecurityPolicy::isPasswordValid(password)) {
        emit commandRejected(QStringLiteral("密码校验失败。"));
        emit logGenerated(QStringLiteral("AUTH"), QString(), QStringLiteral("%1因密码校验失败未执行。").arg(CommandMap::commandName(packet.command)));
        return;
    }

    PendingCommand pending;
    pending.packet = packet;
    pending.frame = ProtocolCodec::encodeWriteFrame(packet.command, packet.data16);
    m_queue.enqueue(pending);
    trySendNext();
}

void ValveTestController::handleAckFrame(const QByteArray &frame)
{
    const QString frameHex = ProtocolCodec::toHexString(frame);
    emit logGenerated(QStringLiteral("RX"), frameHex, QStringLiteral("收到回包帧。"));

    const auto decoded = ProtocolCodec::decodeAckFrame(frame);
    if (!decoded.has_value()) {
        emit logGenerated(QStringLiteral("ERR"), frameHex, QStringLiteral("收到无效回包帧。"));
        emit commandRejected(QStringLiteral("收到无效回包帧。"));
        return;
    }

    emit logGenerated(QStringLiteral("ACK"), frameHex, describeAck(*decoded));

    if (CommandMap::isValveCommand(decoded->command)) {
        const auto resolved = ValveAddressResolver::resolveValveAck(decoded->command, decoded->data16);
        if (resolved.has_value()) {
            emit valveActionConfirmed(resolved->channel, resolved->valveNumbers);
        }
    } else if (decoded->command == CommandMap::kChannelAlarm) {
        emit channelAlarmUpdated(ValveAddressResolver::resolveAbnormalChannels(decoded->data16));
    } else if (decoded->command == CommandMap::kVersionQuery) {
        emit versionReceived(formatVersion(decoded->data16));
    }

    if (!m_inFlight.has_value()) {
        return;
    }

    if (decoded->command != m_inFlight->packet.command || decoded->data16 != m_inFlight->packet.data16) {
        emit logGenerated(
            QStringLiteral("WARN"),
            frameHex,
            QStringLiteral("回包与当前等待确认的命令不匹配。")
        );
        return;
    }

    completeInFlight();
    trySendNext();
}

void ValveTestController::handleTransportError(const QString &message)
{
    emit commandRejected(message);
    emit logGenerated(QStringLiteral("ERR"), QString(), message);
}

void ValveTestController::handleAckTimeout()
{
    if (!m_inFlight.has_value()) {
        return;
    }

    emit commandTimedOut(int(m_inFlight->packet.command), int(m_inFlight->packet.data16));
    emit logGenerated(
        QStringLiteral("TIMEOUT"),
        ProtocolCodec::toHexString(m_inFlight->frame),
        QStringLiteral("%1等待回包超时。").arg(describeOutgoing(m_inFlight->packet))
    );

    completeInFlight();
    trySendNext();
}

void ValveTestController::trySendNext()
{
    if (m_inFlight.has_value() || m_queue.isEmpty()) {
        return;
    }

    if (m_serialService == nullptr || !m_serialService->isOpen()) {
        emit commandRejected(QStringLiteral("串口未连接。"));
        m_queue.clear();
        return;
    }

    PendingCommand pending = m_queue.dequeue();
    if (!m_serialService->writeFrame(pending.frame)) {
        emit commandRejected(QStringLiteral("命令帧发送失败。"));
        emit logGenerated(QStringLiteral("ERR"), ProtocolCodec::toHexString(pending.frame), describeOutgoing(pending.packet));
        trySendNext();
        return;
    }

    m_inFlight = pending;
    emit busyChanged(true);
    emit logGenerated(QStringLiteral("TX"), ProtocolCodec::toHexString(pending.frame), describeOutgoing(pending.packet));
    m_ackTimer.start(m_ackTimeoutMs);
}

void ValveTestController::completeInFlight()
{
    if (!m_inFlight.has_value()) {
        return;
    }

    m_ackTimer.stop();
    m_inFlight.reset();
    emit busyChanged(false);
}

QString ValveTestController::describeOutgoing(const CommandPacket &packet) const
{
    if (CommandMap::isValveCommand(packet.command)) {
        const auto resolved = ValveAddressResolver::resolveValveAck(packet.command, packet.data16);
        if (resolved.has_value() && !resolved->valveNumbers.isEmpty()) {
            return QStringLiteral("请求测试通道 %1 的阀 %2").arg(resolved->channel).arg(resolved->valveNumbers.first());
        }
    }

    if (packet.command == CommandMap::kVersionQuery) {
        return QStringLiteral("请求读取固件版本");
    }

    if (CommandMap::isProtectedTimingCommand(packet.command)) {
        return QStringLiteral("%1设置为 %2，实际下发原始值 %3")
            .arg(CommandMap::commandName(packet.command))
            .arg(formatTimingMilliseconds(packet.data16))
            .arg(packet.data16);
    }

    return QStringLiteral("%1设置为 %2").arg(CommandMap::commandName(packet.command)).arg(packet.data16);
}

QString ValveTestController::describeAck(const DecodedFrame &frame) const
{
    if (CommandMap::isValveCommand(frame.command)) {
        const auto resolved = ValveAddressResolver::resolveValveAck(frame.command, frame.data16);
        if (resolved.has_value() && !resolved->valveNumbers.isEmpty()) {
            QStringList valves;
            valves.reserve(resolved->valveNumbers.size());
            for (const int valveNumber : resolved->valveNumbers) {
                valves.append(QString::number(valveNumber));
            }
            return QStringLiteral("通道 %1 的阀 %2 已确认执行")
                .arg(resolved->channel)
                .arg(valves.join(QStringLiteral(", ")));
        }
        return QStringLiteral("测阀动作已确认。");
    }

    if (frame.command == CommandMap::kChannelAlarm) {
        const QList<int> channels = ValveAddressResolver::resolveAbnormalChannels(frame.data16);
        if (channels.isEmpty()) {
            return QStringLiteral("所有通道状态正常。");
        }

        QStringList parts;
        parts.reserve(channels.size());
        for (const int channel : channels) {
            parts.append(QStringLiteral("通道 %1").arg(channel));
        }
        return QStringLiteral("检测到告警通道：%1").arg(parts.join(QStringLiteral("、")));
    }

    if (frame.command == CommandMap::kVersionQuery) {
        return QStringLiteral("固件版本：%1").arg(formatVersion(frame.data16));
    }

    if (CommandMap::isProtectedTimingCommand(frame.command)) {
        return QStringLiteral("%1已确认：%2，回包原始值 %3")
            .arg(CommandMap::commandName(frame.command))
            .arg(formatTimingMilliseconds(frame.data16))
            .arg(frame.data16);
    }

    return QStringLiteral("%1已确认。").arg(CommandMap::commandName(frame.command));
}

QString ValveTestController::formatVersion(const quint16 data16) const
{
    return QStringLiteral("0x%1").arg(data16, 4, 16, QChar('0')).toUpper();
}
