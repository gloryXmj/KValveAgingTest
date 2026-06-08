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
