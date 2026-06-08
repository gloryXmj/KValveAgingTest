#pragma once

#include "src/core/CommandPacket.h"

#include <QList>

#include <optional>

struct ResolvedValveAck
{
    int channel = 0;
    int segmentIndex = 0;
    QList<int> valveNumbers;
};

class ValveAddressResolver
{
public:
    static std::optional<ResolvedValveAck> resolveValveAck(quint8 command, quint16 data16);
    static std::optional<CommandPacket> buildSingleValveCommand(int channel, int valveNumber);
    static QList<int> resolveAbnormalChannels(quint16 bitmap);
};
