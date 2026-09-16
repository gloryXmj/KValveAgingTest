#pragma once

#include <QMetaType>
#include <QtGlobal>

enum class CommandPurpose
{
    ValveAction,
    SingleValveCycleAction,
    ParameterWrite,
    VersionQuery,
    ChannelAlarm
};

struct CommandPacket
{
    quint8 command = 0;
    quint16 data16 = 0;
    CommandPurpose purpose = CommandPurpose::ParameterWrite;
    quint64 requestId = 0;
};

Q_DECLARE_METATYPE(CommandPacket)
