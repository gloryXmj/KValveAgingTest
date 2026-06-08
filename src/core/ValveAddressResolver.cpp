#include "src/core/ValveAddressResolver.h"

#include "src/core/CommandMap.h"

std::optional<ResolvedValveAck> ValveAddressResolver::resolveValveAck(const quint8 command, const quint16 data16)
{
    if (!CommandMap::isValveCommand(command)) {
        return std::nullopt;
    }

    const int zeroBased = int(command) - int(CommandMap::kValveCommandStart);
    ResolvedValveAck resolved;
    resolved.channel = (zeroBased / CommandMap::kSegmentsPerChannel) + 1;
    resolved.segmentIndex = zeroBased % CommandMap::kSegmentsPerChannel;

    const int segmentStartValve = resolved.segmentIndex * CommandMap::kValvesPerSegment;
    for (int bit = 0; bit < CommandMap::kValvesPerSegment; ++bit) {
        if (data16 & (quint16(1) << bit)) {
            resolved.valveNumbers.append(segmentStartValve + bit + 1);
        }
    }

    return resolved;
}

std::optional<CommandPacket> ValveAddressResolver::buildSingleValveCommand(const int channel, const int valveNumber)
{
    if (channel < 1 || channel > CommandMap::kChannelCountSupported) {
        return std::nullopt;
    }

    if (valveNumber < 1 || valveNumber > CommandMap::kValvesPerChannel) {
        return std::nullopt;
    }

    const int zeroBasedValve = valveNumber - 1;
    const int segmentIndex = zeroBasedValve / CommandMap::kValvesPerSegment;
    const int bitIndex = zeroBasedValve % CommandMap::kValvesPerSegment;

    CommandPacket packet;
    packet.command = quint8(
        int(CommandMap::kValveCommandStart)
        + ((channel - 1) * CommandMap::kSegmentsPerChannel)
        + segmentIndex
    );
    packet.data16 = quint16(1u << bitIndex);
    packet.purpose = CommandPurpose::ValveAction;
    return packet;
}

QList<int> ValveAddressResolver::resolveAbnormalChannels(const quint16 bitmap)
{
    QList<int> channels;
    for (int bit = 0; bit < CommandMap::kChannelCountSupported; ++bit) {
        if (bitmap & (quint16(1) << bit)) {
            channels.append(bit + 1);
        }
    }
    return channels;
}
