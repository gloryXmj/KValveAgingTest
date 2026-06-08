#pragma once

#include "src/serial/SerialPortTypes.h"

#include <QByteArray>
#include <QSettings>
#include <QString>

class AppSettings
{
public:
    explicit AppSettings(
        const QString &organization = QStringLiteral("Keye"),
        const QString &application = QStringLiteral("KValveAgingTestTool")
    );

    SerialPortSettings loadSerialSettings() const;
    void saveSerialSettings(const SerialPortSettings &settings) const;
    int loadVisibleValveCount() const;
    void saveVisibleValveCount(int count) const;

    QByteArray loadWindowGeometry() const;
    void saveWindowGeometry(const QByteArray &geometry) const;

private:
    mutable QSettings m_settings;
};
