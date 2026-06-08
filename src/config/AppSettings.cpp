#include "src/config/AppSettings.h"

#include "src/core/CommandMap.h"

AppSettings::AppSettings(const QString &organization, const QString &application)
    : m_settings(organization, application)
{
}

SerialPortSettings AppSettings::loadSerialSettings() const
{
    SerialPortSettings settings;
    settings.portName = m_settings.value(QStringLiteral("serial/portName")).toString();
    settings.baudRate = m_settings.value(QStringLiteral("serial/baudRate"), 115200).toInt();
    settings.dataBits = m_settings.value(QStringLiteral("serial/dataBits"), 8).toInt();
    settings.stopBits = m_settings.value(QStringLiteral("serial/stopBits"), 1).toInt();
    settings.parity = m_settings.value(QStringLiteral("serial/parity"), 0).toInt();
    settings.flowControl = m_settings.value(QStringLiteral("serial/flowControl"), 0).toInt();
    return settings;
}

void AppSettings::saveSerialSettings(const SerialPortSettings &settings) const
{
    m_settings.setValue(QStringLiteral("serial/portName"), settings.portName);
    m_settings.setValue(QStringLiteral("serial/baudRate"), settings.baudRate);
    m_settings.setValue(QStringLiteral("serial/dataBits"), settings.dataBits);
    m_settings.setValue(QStringLiteral("serial/stopBits"), settings.stopBits);
    m_settings.setValue(QStringLiteral("serial/parity"), settings.parity);
    m_settings.setValue(QStringLiteral("serial/flowControl"), settings.flowControl);
}

ControlParameters AppSettings::loadControlParameters() const
{
    ControlParameters parameters;
    parameters.valveSwitch = m_settings.value(QStringLiteral("control/valveSwitch"), parameters.valveSwitch).toInt();
    parameters.triggerMode = m_settings.value(QStringLiteral("control/triggerMode"), parameters.triggerMode).toInt();
    parameters.blowCount = m_settings.value(QStringLiteral("control/blowCount"), parameters.blowCount).toInt();
    parameters.blowIntervalMs = m_settings.value(QStringLiteral("control/blowIntervalMs"), parameters.blowIntervalMs).toInt();
    parameters.blowTimeMs = m_settings.value(QStringLiteral("control/blowTimeMs"), parameters.blowTimeMs).toDouble();
    parameters.chargeTimeMs = m_settings.value(QStringLiteral("control/chargeTimeMs"), parameters.chargeTimeMs).toDouble();
    parameters.stopChargeTimeMs = m_settings.value(QStringLiteral("control/stopChargeTimeMs"), parameters.stopChargeTimeMs).toDouble();
    parameters.rechargeTimeMs = m_settings.value(QStringLiteral("control/rechargeTimeMs"), parameters.rechargeTimeMs).toDouble();
    parameters.channelCount = m_settings.value(QStringLiteral("control/channelCount"), parameters.channelCount).toInt();
    parameters.independentChannelEnable = m_settings.value(
        QStringLiteral("control/independentChannelEnable"),
        parameters.independentChannelEnable
    ).toInt();
    return parameters;
}

void AppSettings::saveControlParameters(const ControlParameters &parameters) const
{
    m_settings.setValue(QStringLiteral("control/valveSwitch"), parameters.valveSwitch);
    m_settings.setValue(QStringLiteral("control/triggerMode"), parameters.triggerMode);
    m_settings.setValue(QStringLiteral("control/blowCount"), parameters.blowCount);
    m_settings.setValue(QStringLiteral("control/blowIntervalMs"), parameters.blowIntervalMs);
    m_settings.setValue(QStringLiteral("control/blowTimeMs"), parameters.blowTimeMs);
    m_settings.setValue(QStringLiteral("control/chargeTimeMs"), parameters.chargeTimeMs);
    m_settings.setValue(QStringLiteral("control/stopChargeTimeMs"), parameters.stopChargeTimeMs);
    m_settings.setValue(QStringLiteral("control/rechargeTimeMs"), parameters.rechargeTimeMs);
    m_settings.setValue(QStringLiteral("control/channelCount"), parameters.channelCount);
    m_settings.setValue(QStringLiteral("control/independentChannelEnable"), parameters.independentChannelEnable);
}

int AppSettings::loadVisibleValveCount() const
{
    return m_settings.value(
        QStringLiteral("ui/visibleValveCount"),
        CommandMap::kValvesPerChannel
    ).toInt();
}

void AppSettings::saveVisibleValveCount(const int count) const
{
    m_settings.setValue(QStringLiteral("ui/visibleValveCount"), count);
}

QByteArray AppSettings::loadWindowGeometry() const
{
    return m_settings.value(QStringLiteral("window/geometry")).toByteArray();
}

void AppSettings::saveWindowGeometry(const QByteArray &geometry) const
{
    m_settings.setValue(QStringLiteral("window/geometry"), geometry);
}
