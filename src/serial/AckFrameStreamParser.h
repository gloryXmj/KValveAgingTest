#pragma once

#include <QByteArray>
#include <QList>

class AckFrameStreamParser
{
public:
    QList<QByteArray> pushBytes(const QByteArray &data);
    void reset();

private:
    QByteArray m_buffer;
};
