#include <EventLoop/ListeningSocket.hpp>

#include <netdb.h>

#include <EventLoop/Runner.hpp>
#include <EventLoop/SocketAddress.hpp>
#include <EventLoop/DumpUtils.hpp>

#include "private/System.hpp"

using namespace EventLoop;

ListeningSocket::ListeningSocket(Runner                     &runner,
                                 int                        type,
                                 int                        protocol,
                                 const SocketAddress        &sa,
                                 const NewAcceptedSocketCb  &newAcceptedSocketCb):
    ListeningSocket(System::getSystem(), runner, type, protocol, sa, newAcceptedSocketCb, HandleExceptionCb()) { }

ListeningSocket::ListeningSocket(Runner                     &runner,
                                 int                        type,
                                 int                        protocol,
                                 const SocketAddress        &sa,
                                 const NewAcceptedSocketCb  &newAcceptedSocketCb,
                                 const HandleExceptionCb    &handleExceptionCb):
    ListeningSocket(System::getSystem(), runner, type, protocol, sa, newAcceptedSocketCb, handleExceptionCb) { }

ListeningSocket::ListeningSocket(Runner                     &runner,
                                 FileDescriptor             &fd,
                                 const NewAcceptedSocketCb  &newAcceptedSocketCb):
    ListeningSocket(System::getSystem(), runner, fd, newAcceptedSocketCb, HandleExceptionCb()) { }

ListeningSocket::ListeningSocket(Runner                     &runner,
                                 FileDescriptor             &fd,
                                 const NewAcceptedSocketCb  &newAcceptedSocketCb,
                                 const HandleExceptionCb    &handleExceptionCb):
    ListeningSocket(System::getSystem(), runner, fd, newAcceptedSocketCb, handleExceptionCb) { }

ListeningSocket::ListeningSocket(System                     &system,
                                 Runner                     &runner,
                                 int                        type,
                                 int                        protocol,
                                 const SocketAddress        &sa,
                                 const NewAcceptedSocketCb  &newAcceptedSocketCb,
                                 const HandleExceptionCb    &handleExceptionCb):
    system(system),
    runner(runner),
    fd(system, system.socket(sa.getFamily(), type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol)),
    newAcceptedSocketCb(newAcceptedSocketCb),
    handleExceptionCb(handleExceptionCb)
{
    system.setReuseAddr(fd);
    system.bind(fd, &sa.getAddress(), sa.getUsedSize());
    system.listen(fd, 16);
    runner.addFDListener(*this, fd, EVENT_IN);
}

ListeningSocket::ListeningSocket(System                     &system,
                                 Runner                     &runner,
                                 FileDescriptor             &fd,
                                 const NewAcceptedSocketCb  &newAcceptedSocketCb,
                                 const HandleExceptionCb    &handleExceptionCb):
    system(system),
    runner(runner),
    fd(fd.release()),
    newAcceptedSocketCb(newAcceptedSocketCb),
    handleExceptionCb(handleExceptionCb)
{
    system.setCloseOnExec(this->fd);
    system.setNonBlock(this->fd);
    runner.addFDListener(*this, this->fd, EVENT_IN);
}

ListeningSocket::~ListeningSocket()
{
    runner.delFDListener(fd);
}

SocketAddress ListeningSocket::getName() const
{
    SocketAddress sa;
    system.getSockName(fd, sa.getPointer(), sa.getSizePointer());
    return sa;
}

void ListeningSocket::handleEventsCb(int,
                                     unsigned int)
{
    bool more;
    do
    {
        if (handleExceptionCb)
        {
            more = handleEventsOrException();
        }
        else
        {
            more = accept();
        }
    }
    while (more);
}

bool ListeningSocket::handleEventsOrException()
{
    try
    {
        return accept();
    }
    catch (const std::exception &e)
    {
        handleExceptionCb(e);
        return false;
    }
}

bool ListeningSocket::accept()
{
    SocketAddress sa;
    FileDescriptor fd(system, system.accept(this->fd, sa.getPointer(), sa.getSizePointer()));
    if (fd)
    {
        newAcceptedSocketCb(fd, sa);
        return true;
    }
    return false;
}

void ListeningSocket::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, system);
    DUMP_ADDRESS_OF(os, runner);
    DUMP_VALUE_OF(os, fd);
    DUMP_VALUE_OF(os, newAcceptedSocketCb);
    DUMP_VALUE_OF(os, handleExceptionCb);
    DUMP_RESULT_OF(os, getName());
    DUMP_AS(os, FDListener);
    DUMP_END(os);
}
