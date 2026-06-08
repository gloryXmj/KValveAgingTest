#include "src/app/ProtocolConsoleLogger.h"

#include <QDateTime>
#include <QTextStream>

ProtocolConsoleLogger::ProtocolConsoleLogger(QObject *parent)
    : QObject(parent)
{
}

void ProtocolConsoleLogger::printLogLine(const QString &direction, const QString &hex, const QString &description)
{
    const QString line = QStringLiteral("%1 | %2 | %3 | %4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")), direction, hex, description);

    QTextStream stream(stdout);
    stream << line << Qt::endl;
    stream << line << '\n';
    stream.flush();
}
