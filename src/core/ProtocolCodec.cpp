#include "src/core/ProtocolCodec.h"

#include "src/core/CommandMap.h"

#include <QStringList>

QByteArray ProtocolCodec::encodeWriteFrame(const quint8 command, const quint16 data16)
{
    QByteArray frame;
    frame.reserve(4);
    frame.append(char(CommandMap::writeHeaderForCommand(command)));
    frame.append(char(command));
    frame.append(char((data16 >> 8) & 0xFF));
    frame.append(char(data16 & 0xFF));
    return frame;
}

std::optional<DecodedFrame> ProtocolCodec::decodeAckFrame(const QByteArray &frame)
{
    if (frame.size() != 4) {
        return std::nullopt;
    }

    const quint8 header = quint8(frame.at(0));
    const quint8 command = quint8(frame.at(1));
    if (!CommandMap::isReplyHeader(header) || !CommandMap::isKnownCommand(command)) {
        return std::nullopt;
    }

    DecodedFrame decoded;
    decoded.header = header;
    decoded.command = command;
    decoded.data16 = (quint16(quint8(frame.at(2))) << 8) | quint16(quint8(frame.at(3)));
    return decoded;
}

QString ProtocolCodec::toHexString(const QByteArray &frame)
{
    QStringList parts;
    parts.reserve(frame.size());
    for (const char byte : frame) {
        parts.append(QStringLiteral("%1").arg(quint8(byte), 2, 16, QChar('0')).toUpper());
    }
    return parts.join(QStringLiteral(" "));
}
