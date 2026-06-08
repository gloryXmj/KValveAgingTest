#pragma once

#include <QObject>
#include <QString>

class ProtocolConsoleLogger : public QObject
{
    Q_OBJECT

public:
    explicit ProtocolConsoleLogger(QObject *parent = nullptr);

public slots:
    void printLogLine(const QString &direction, const QString &hex, const QString &description);
};
