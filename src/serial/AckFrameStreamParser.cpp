#include "src/serial/AckFrameStreamParser.h"

#include "src/core/CommandMap.h"

namespace
{
bool isPotentialReplyFrame(const QByteArray &buffer, const int offset)
{
    if (offset < 0 || offset + 4 > buffer.size()) {
        return false;
    }

    const quint8 header = quint8(buffer.at(offset));
    const quint8 command = quint8(buffer.at(offset + 1));
    return CommandMap::isReplyHeader(header) && CommandMap::isKnownCommand(command);
}
}

QList<QByteArray> AckFrameStreamParser::pushBytes(const QByteArray &data)
{
    m_buffer.append(data);

    QList<QByteArray> frames;
    while (true) {
        if (m_buffer.size() < 4) {
            break;
        }

        int frameStart = -1;
        const int lastCandidate = m_buffer.size() - 4;
        for (int index = 0; index <= lastCandidate; ++index) {
            if (isPotentialReplyFrame(m_buffer, index)) {
                frameStart = index;
                break;
            }
        }

        if (frameStart < 0) {
            if (m_buffer.size() > 3) {
                m_buffer.remove(0, m_buffer.size() - 3);
            }
            break;
        }

        if (frameStart > 0) {
            m_buffer.remove(0, frameStart);
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
