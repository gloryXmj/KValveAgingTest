#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

struct SerialPortDescriptor
{
    QString name;
    QString description;
    QString hardwareId;
};

struct SerialPortSettings
{
    QString portName;
    int baudRate = 9600;
    int dataBits = 8;
    int stopBits = 1;
    int parity = 0;
    int flowControl = 0;
};

Q_DECLARE_METATYPE(SerialPortDescriptor)
Q_DECLARE_METATYPE(SerialPortSettings)
Q_DECLARE_METATYPE(QVector<SerialPortDescriptor>)
