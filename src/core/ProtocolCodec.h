#pragma once

#include <QByteArray>
#include <QString>

#include <optional>

struct DecodedFrame
{
    quint8 header = 0;
    quint8 command = 0;
    quint16 data16 = 0;
};

class ProtocolCodec
{
public:
    static QByteArray encodeWriteFrame(quint8 command, quint16 data16);
    static std::optional<DecodedFrame> decodeAckFrame(const QByteArray &frame);
    static QString toHexString(const QByteArray &frame);
};
