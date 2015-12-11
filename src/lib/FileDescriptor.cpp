#include <EventLoop/FileDescriptor.hpp>

#include <utility>

#include "private/System.hpp"

using namespace EventLoop;

FileDescriptor::FileDescriptor() noexcept:
    FileDescriptor(System::getSystem(), -1) { }

FileDescriptor::FileDescriptor(int fd) noexcept:
    FileDescriptor(System::getSystem(), fd) { }

FileDescriptor::FileDescriptor(System &system) noexcept:
    FileDescriptor(system, -1) { }

FileDescriptor::FileDescriptor(System &system, int fd) noexcept:
    system(system),
    fd(fd)
{
}

FileDescriptor::~FileDescriptor() noexcept
{
    if (fd >= 0)
    {
        system.close(fd);
    }
}

void FileDescriptor::close()
{
    if (fd < 0) throw InvalidFD();
    system.close(fd);
    fd = -1;
}

int FileDescriptor::release()
{
    if (fd < 0) throw InvalidFD();
    const int ret(fd);
    fd = -1;
    return ret;
}

void FileDescriptor::swap(FileDescriptor &other)
{
    std::swap(fd, other.fd);
}

FileDescriptor & FileDescriptor::operator = (int fd) noexcept
{
    if (this->fd >= 0)
    {
        system.close(this->fd);
    }
    this->fd = fd;
    return *this;
}

std::ostream & EventLoop::operator << (std::ostream         &out,
                                       const FileDescriptor &fd)
{
    out << fd.fd;
    return out;
}
