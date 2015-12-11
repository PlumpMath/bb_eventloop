#include <EventLoop/ControlMessage.hpp>

using namespace EventLoop;

ControlMessage::ControlMessage(const void   *data,
                               size_type    size):
    usedSize(size)
{
    const char *begin(static_cast<const char *>(data));
    const char *end(begin + size);
    buffer.insert(buffer.begin(), begin, end);
}

void ControlMessage::markUsed(size_type size)
{
    if (buffer.size() < size)
    {
        throw Overflow();
    }
    usedSize = size;
}
