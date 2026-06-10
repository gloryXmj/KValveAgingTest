#pragma once

#include "src/core/CommandMap.h"

#include <QMetaType>

struct ControlParameters
{
    int operationMode = 0;
    int valveSwitch = 0;
    int triggerMode = 0;
    int blowCount = 1;
    int blowIntervalMs = 10;
    int testFrequencyHz = 100;
    int agingFrequencyHz = 100;
    double blowTimeMs = 2.0;
    double chargeTimeMs = 1.0;
    double stopChargeTimeMs = 1.5;
    double rechargeTimeMs = 0.4;
    int channelCount = CommandMap::kChannelCountSupported;
    int independentChannelEnable = 0;
};

Q_DECLARE_METATYPE(ControlParameters)
