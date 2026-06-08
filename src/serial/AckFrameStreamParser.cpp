#include "src/serial/AckFrameStreamParser.h"

#include "src/core/CommandMap.h"

QList<QByteArray> AckFrameStreamParser::pushBytes(const QByteArray &data)
{
    m_buffer.append(data);

    QList<QByteArray> frames;
    while (true) {
        const int headerIndex = m_buffer.indexOf(char(CommandMap::kAckHeader));
        if (headerIndex < 0) {
            m_buffer.clear();
            break;
        }

        if (headerIndex > 0) {
            m_buffer.remove(0, headerIndex);
        }

        if (m_buffer.size() < 4) {
            break;
        }

        frames.append(m_buffer.left(4));
        m_buffer.remove(0, 4);
    }

    return frames;
}

void AckFrameStreamParser::reset()
{
    m_buffer.clear();
}
