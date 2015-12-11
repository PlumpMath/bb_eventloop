#include <EventLoop/StreamBuffer.hpp>

#include <ostream>
#include <cstring>
#include <climits>

#include <EventLoop/DumpUtils.hpp>

#define BUFFER_ALIGN_TO static_cast<size_t>(8U)

using namespace EventLoop;

StreamBuffer::Buffer::Buffer(size_t needSize)
{
    const size_t tmp(BUFFER_ALIGN_TO - 1U);
    size = (needSize + tmp) & ~tmp;
    base = boost::shared_array<uint64_t>(new uint64_t[size / BUFFER_ALIGN_TO]);
    data = static_cast<void *>(base.get());
}

StreamBuffer::StreamBuffer():
    usedSize(0),
    totalSize(0),
    iovecArraySize(0)
{
}

StreamBuffer::StreamBuffer(const StreamBuffer &sb):
    buffers(sb.buffers),
    usedSize(sb.usedSize),
    totalSize(sb.totalSize),
    iovecArraySize(0)
{
}

StreamBuffer &StreamBuffer::operator = (const StreamBuffer &sb)
{
    buffers = sb.buffers;
    usedSize = sb.usedSize;
    totalSize = sb.totalSize;
    return *this;
}

StreamBuffer::~StreamBuffer()
{
}

void StreamBuffer::allocate(size_t size)
{
    if (getFreeSize() < size)
    {
        buffers.push_back(Buffer(size - getFreeSize()));
        totalSize += buffers.back().size;
    }
}

StreamBuffer StreamBuffer::spliceHead(size_t size)
{
    if (size > totalSize)
    {
        throw Underflow();
    }

    StreamBuffer sb;
    sb.usedSize = std::min(size, usedSize);
    if (size <= usedSize)
    {
        usedSize -= size;
    }
    else
    {
        usedSize = 0;
    }
    totalSize -= size;

    sb.totalSize = size;

    size_t sizeLeft(size);

    while (sizeLeft > 0)
    {
        Buffer &buffer(buffers.front());
        if (buffer.size <= sizeLeft)
        {
            sb.buffers.push_back(buffer);
            sizeLeft -= buffer.size;
            buffers.pop_front();
        }
        else
        {
            sb.buffers.push_back(buffer);
            sb.buffers.back().size = sizeLeft;
            buffer.data = incrementPointer(buffer.data, sizeLeft);
            buffer.size -= sizeLeft;
            sizeLeft = 0;
        }
    }

    return sb;
}

void StreamBuffer::cutHead(size_t size)
{
    if (size > totalSize)
    {
        throw Underflow();
    }

    if (size <= usedSize)
    {
        usedSize -= size;
    }
    else
    {
        usedSize = 0;
    }
    totalSize -= size;

    size_t sizeLeft(size);

    while (sizeLeft > 0)
    {
        Buffer &buffer(buffers.front());
        if (buffer.size <= sizeLeft)
        {
            sizeLeft -= buffer.size;
            buffers.pop_front();
        }
        else
        {
            buffer.data = incrementPointer(buffer.data, sizeLeft);
            buffer.size -= sizeLeft;
            sizeLeft = 0;
        }
    }
}

void StreamBuffer::allocateIOVecArray()
{
    const size_t buffersSize(buffers.size());
    if (iovecArraySize < buffersSize)
    {
        iovecArray.reset(new struct iovec[buffersSize]);
        for (size_t i = 0U; i < buffersSize; ++i)
        {
            iovecArray[i].iov_base = nullptr;
            iovecArray[i].iov_len = 0U;
        }
        iovecArraySize = buffersSize;
    }
}

StreamBuffer::IOVecArray StreamBuffer::getUsedIOVec()
{
    allocateIOVecArray();

    Buffers::const_iterator i(buffers.begin());

    size_t dataLeft(usedSize);
    size_t size(0);
    while ((dataLeft > 0) &&
           (size < IOV_MAX))
    {
        size_t usedSize(std::min(i->size, dataLeft));
        iovecArray[size].iov_base = i->data;
        iovecArray[size].iov_len = usedSize;
        dataLeft -= usedSize;
        ++i;
        ++size;
    }

    return std::make_pair(size == 0 ? nullptr : iovecArray.get(), size);
}

StreamBuffer::IOVecArray StreamBuffer::getFreeIOVec()
{
    allocateIOVecArray();

    const Location location(findFirstUnused());
    Buffers::const_iterator i(pointedBuffer(location));
    size_t size(0);
    if (i != buffers.end())
    {
        iovecArray[size].iov_base = pointerInBuffer(location);
        iovecArray[size].iov_len = sizeLeftInBuffer(location);
        ++size;
        for (++i;
             (i != buffers.end()) && (size < IOV_MAX);
             ++i, ++size)
        {
            iovecArray[size].iov_base = i->data;
            iovecArray[size].iov_len = i->size;
        }
    }

    return std::make_pair(size == 0 ? nullptr : iovecArray.get(), size);
}

void StreamBuffer::appendData(const void    *data,
                              size_t        size)
{
    allocate(size);

    const char *copyFrom(static_cast<const char *>(data));
    size_t copyLeft(size);
    Location location(findFirstUnused());

    const size_t toCopy(std::min(sizeLeftInBuffer(location), copyLeft));
    memcpy(pointerInBuffer(location), copyFrom, toCopy);
    copyLeft -= toCopy;
    copyFrom += toCopy;

    Buffers::const_iterator i(pointedBuffer(location));
    while (copyLeft > 0)
    {
        ++i;
        const size_t toCopy(std::min(i->size, copyLeft));
        memcpy(i->data, copyFrom, toCopy);
        copyLeft -= toCopy;
        copyFrom += toCopy;
    }

    increaseUsed(size);
}

void StreamBuffer::appendStreamBuffer(const StreamBuffer &sb)
{
    const Location location(findFirstUnused());
    const Buffers::iterator insertBefore(pointedBuffer(location));
    if ((insertBefore != buffers.end()) &&
        (sizeLeftInBuffer(location) > 0))
    {
        Buffers::iterator newBuffer(buffers.insert(insertBefore, *insertBefore));
        const size_t size(offSetInBuffer(location));
        newBuffer->size = size;
        insertBefore->size -= size;
        insertBefore->data = incrementPointer(insertBefore->data, size);
    }
    size_t dataLeft(sb.usedSize);
    Buffers::const_iterator i(sb.buffers.begin());
    while (dataLeft > 0)
    {
        size_t dataToCopy(std::min(dataLeft, i->size));
        Buffers::iterator newBuffer(buffers.insert(insertBefore, *i));
        newBuffer->size = dataToCopy;
        usedSize += dataToCopy;
        totalSize += dataToCopy;
        dataLeft -= dataToCopy;
        ++i;
    }
}

void *StreamBuffer::getMutableArray(size_t size)
{
    const Location location(findFirstUnused());
    if (pointedBuffer(location) == buffers.end())
    {
        return getMutableArrayNoFreeBuffers(size);
    }
    else if (sizeLeftInBuffer(location) >= size)
    {
        usedSize += size;
        return pointerInBuffer(location);
    }
    else
    {
        return getMutableArrayFirstUnusedBufferTooSmall(location, size);
    }
}

void *StreamBuffer::getMutableArrayNoFreeBuffers(size_t size)
{
    allocate(size);
    usedSize += size;
    return buffers.back().data;
}

void *StreamBuffer::getMutableArrayFirstUnusedBufferTooSmall(const Location     &location,
                                                             size_t             size)
{
    Buffers::iterator i(pointedBuffer(location));
    if (i->data != pointerInBuffer(location))
    {
        const size_t toTruncate(sizeLeftInBuffer(location));
        i->size -= toTruncate;
        totalSize -= toTruncate;
        ++i;
    }
    const Buffers::iterator newBufferI(buffers.insert(i, Buffer(size)));
    totalSize += newBufferI->size;
    usedSize += size;

    return newBufferI->data;
}

void StreamBuffer::linearize()
{
    if ((usedSize > 0) &&
        ((buffers.front().size < usedSize) ||
         ((((uintptr_t)buffers.front().data) % boost::alignment_of<uint64_t>()) != 0)))
    {
        forceLinearize();
    }
}

void StreamBuffer::forceLinearize()
{
    buffers.push_back(Buffer(usedSize));
    char *ptr(static_cast<char *>(buffers.back().data));
    size_t copyLeft(usedSize);

    while (copyLeft > 0)
    {
        size_t toCopy(std::min(copyLeft, buffers.front().size));
        memcpy(ptr, buffers.front().data, toCopy);
        copyLeft -= toCopy;
        ptr += toCopy;
        buffers.pop_front();
    }
    totalSize = buffers.back().size;
}

const void *StreamBuffer::getConstArray(size_t  offset,
                                        size_t  size)
{
    if (offset + size > usedSize)
    {
        throw Overflow();
    }

    const Location location(findLocation(offset));
    if (sizeLeftInBuffer(location) >= size)
    {
        return pointerInBuffer(location);
    }
    else
    {
        forceLinearize();
        return incrementPointer(buffers.front().data, offset);
    }
}

StreamBuffer::Location StreamBuffer::findLocation(size_t size) const
{
    /* Bit ugly: this function has to be const, but can't return const iterator */
    Buffers::iterator i(const_cast<Buffers &>(buffers).begin());
    size_t dataLeft(size);
    while (dataLeft > 0)
    {
        if (i->size > dataLeft)
        {
            return std::make_pair(i, dataLeft);
        }
        dataLeft -= i->size;
        ++i;
    }
    return std::make_pair(i, static_cast<size_t>(0));
}

StreamBuffer::Location StreamBuffer::findFirstUnused() const
{
    return findLocation(usedSize);
}

void StreamBuffer::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_BLOCK_BEGIN(os, "buffers");
    for (Buffers::const_iterator i = buffers.begin();
         i != buffers.end();
         ++i)
    {
        os << "base: " << i->base.get() << '\n';
        os << "data: " << i->data << '\n';
        os << "size: " << i->size << '\n';
    }
    DUMP_BLOCK_END(os);
    DUMP_VALUE_OF(os, usedSize);
    DUMP_VALUE_OF(os, totalSize);
    DUMP_BLOCK_BEGIN(os, "iovecArray");
    for (size_t i = 0U;
         i != iovecArraySize;
         ++i)
    {
        os << "iov_base: " << iovecArray[i].iov_base << '\n';
        os << "iov_len: " << iovecArray[i].iov_len << '\n';
    }
    DUMP_BLOCK_END(os);
    DUMP_VALUE_OF(os, iovecArraySize);
    DUMP_END(os);
}
