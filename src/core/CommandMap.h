#pragma once

#include <QString>
#include <QtGlobal>

namespace CommandMap
{
constexpr quint8 kWriteHeader = 0xA0;
constexpr quint8 kAckHeader = 0xAA;

constexpr quint8 kValveSwitch = 0x00;
constexpr quint8 kTriggerMode = 0x01;
constexpr quint8 kBlowCount = 0x02;
constexpr quint8 kBlowInterval = 0x03;
constexpr quint8 kBlowTime = 0x05;
constexpr quint8 kChargeTime = 0x06;
constexpr quint8 kStopChargeTime = 0x07;
constexpr quint8 kRechargeTime = 0x08;
constexpr quint8 kChannelCount = 0x09;
constexpr quint8 kChannelAlarm = 0xF0;
constexpr quint8 kVersionQuery = 0xFF;

constexpr quint8 kValveCommandStart = 0x50;
constexpr quint8 kValveCommandEnd = 0x8F;

constexpr int kChannelCountSupported = 8;
constexpr int kValvesPerSegment = 16;
constexpr int kSegmentsPerChannel = 8;
constexpr int kValvesPerChannel = kValvesPerSegment * kSegmentsPerChannel;

inline bool isValveCommand(const quint8 command)
{
    return command >= kValveCommandStart && command <= kValveCommandEnd;
}

inline bool isProtectedTimingCommand(const quint8 command)
{
    return command == kBlowTime
        || command == kChargeTime
        || command == kStopChargeTime
        || command == kRechargeTime;
}

inline QString commandName(const quint8 command)
{
    switch (command) {
    case kValveSwitch:
        return QStringLiteral("阀开关");
    case kTriggerMode:
        return QStringLiteral("触发模式");
    case kBlowCount:
        return QStringLiteral("吹气次数");
    case kBlowInterval:
        return QStringLiteral("吹气间隔");
    case kBlowTime:
        return QStringLiteral("吹气时间");
    case kChargeTime:
        return QStringLiteral("充电时间");
    case kStopChargeTime:
        return QStringLiteral("停止充电时间");
    case kRechargeTime:
        return QStringLiteral("续充电时间");
    case kChannelCount:
        return QStringLiteral("通道数量");
    case kChannelAlarm:
        return QStringLiteral("通道告警");
    case kVersionQuery:
        return QStringLiteral("版本查询");
    default:
        break;
    }

    if (isValveCommand(command)) {
        return QStringLiteral("测阀动作");
    }

    return QStringLiteral("未知命令");
}
}
