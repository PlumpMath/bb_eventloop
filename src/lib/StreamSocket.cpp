#include <EventLoop/StreamSocket.hpp>

#include <ostream>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>

#include <EventLoop/Runner.hpp>
#include <EventLoop/SystemException.hpp>
#include <EventLoop/SocketAddress.hpp>
#include <EventLoop/DumpUtils.hpp>

#include "private/System.hpp"

using namespace EventLoop;

namespace
{
    const int TYPE(SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC);
}

StreamSocket::StreamSocket(Runner                           &runner,
                           int                              protocol,
                           const SocketAddress              &local,
                           const SocketAddress              &remote,
                           const ConnectionEstablishedCb    &connectionEstablishedCb,
                           const ConnectionClosedCb         &connectionClosedCb,
                           const DataReceivedCb             &dataReceivedCb,
                           const AllDataSentCb              &allDataSentCb):
    StreamSocket(System::getSystem(), runner, protocol, local, remote, connectionEstablishedCb, connectionClosedCb, dataReceivedCb, allDataSentCb) { }

StreamSocket::StreamSocket(Runner                           &runner,
                           int                              protocol,
                           const SocketAddress              &remote,
                           const ConnectionEstablishedCb    &connectionEstablishedCb,
                           const ConnectionClosedCb         &connectionClosedCb,
                           const DataReceivedCb             &dataReceivedCb,
                           const AllDataSentCb              &allDataSentCb):
    StreamSocket(System::getSystem(), runner, protocol, remote, connectionEstablishedCb, connectionClosedCb, dataReceivedCb, allDataSentCb) { }

StreamSocket::StreamSocket(Runner                   &runner,
                           FileDescriptor           &fd,
                           const ConnectionClosedCb &connectionClosedCb,
                           const DataReceivedCb     &dataReceivedCb,
                           const AllDataSentCb      &allDataSentCb):
    StreamSocket(System::getSystem(), runner, fd, connectionClosedCb, dataReceivedCb, allDataSentCb) { }

StreamSocket::StreamSocket(System                           &system,
                           Runner                           &runner,
                           int                              protocol,
                           const SocketAddress              &local,
                           const SocketAddress              &remote,
                           const ConnectionEstablishedCb    &connectionEstablishedCb,
                           const ConnectionClosedCb         &connectionClosedCb,
                           const DataReceivedCb             &dataReceivedCb,
                           const AllDataSentCb              &allDataSentCb):
    system(system),
    runner(runner),
    fd(system, system.socket(remote.getFamily(), TYPE, protocol)),
    connecting(true),
    sending(false),
    connectionEstablishedCb(connectionEstablishedCb),
    connectionClosedCb(connectionClosedCb),
    dataReceivedCb(dataReceivedCb),
    allDataSentCb(allDataSentCb)
{
    system.bind(fd, &local.getAddress(), local.getUsedSize());
    system.connect(fd, &remote.getAddress(), remote.getUsedSize());
    runner.addFDListener(*this, fd, EVENT_IN | EVENT_OUT);
}

StreamSocket::StreamSocket(System                           &system,
                           Runner                           &runner,
                           int                              protocol,
                           const SocketAddress              &remote,
                           const ConnectionEstablishedCb    &connectionEstablishedCb,
                           const ConnectionClosedCb         &connectionClosedCb,
                           const DataReceivedCb             &dataReceivedCb,
                           const AllDataSentCb              &allDataSentCb):
    system(system),
    runner(runner),
    fd(system, system.socket(remote.getFamily(), TYPE, protocol)),
    connecting(true),
    sending(false),
    connectionEstablishedCb(connectionEstablishedCb),
    connectionClosedCb(connectionClosedCb),
    dataReceivedCb(dataReceivedCb),
    allDataSentCb(allDataSentCb)
{
    system.connect(fd, &remote.getAddress(), remote.getUsedSize());
    runner.addFDListener(*this, fd, EVENT_IN | EVENT_OUT);
}

StreamSocket::StreamSocket(System                   &system,
                           Runner                   &runner,
                           FileDescriptor           &fd,
                           const ConnectionClosedCb &connectionClosedCb,
                           const DataReceivedCb     &dataReceivedCb,
                           const AllDataSentCb      &allDataSentCb):
    system(system),
    runner(runner),
    fd(system, fd.release()),
    connecting(false),
    sending(false),
    connectionClosedCb(connectionClosedCb),
    dataReceivedCb(dataReceivedCb),
    allDataSentCb(allDataSentCb)
{
    system.setCloseOnExec(this->fd);
    system.setNonBlock(this->fd);
    runner.addFDListener(*this, this->fd, EVENT_IN);
}

StreamSocket::~StreamSocket()
{
    if (fd)
    {
        runner.delFDListener(fd);
    }
}

SocketAddress StreamSocket::getName() const
{
    SocketAddress sa;
    system.getSockName(fd, sa.getPointer(), sa.getSizePointer());
    return sa;
}

SocketAddress StreamSocket::getPeer() const
{
    SocketAddress sa;
    system.getPeerName(fd, sa.getPointer(), sa.getSizePointer());
    return sa;
}

void StreamSocket::handleEventsCb(int,
                                  unsigned int  events)
{
    checkConnectionEstablished(events);
    checkIncomingData(events);
    checkOutgoingData(events);
    checkConnectionClosed(events);
}

void StreamSocket::checkConnectionEstablished(unsigned int events)
{
    if ((connecting) &&
        (events & (EVENT_IN | EVENT_OUT)) &&
        (!(events & (EVENT_ERR | EVENT_HUP))))
    {
        connecting = false;
        if (connectionEstablishedCb)
        {
            connectionEstablishedCb();
        }
        if (output.getUsedSize() == 0)
        {
            runner.modifyFDListener(fd, EVENT_IN);
        }
    }
}

void StreamSocket::checkIncomingData(unsigned int events)
{
    try
    {
        if (events & EVENT_IN)
        {
            allocateReceiveBuffer();
            receive();
        }
    }
    catch (const SystemException &e)
    {
        close(e.error);
    }
    catch (const std::bad_alloc &)
    {
        close(ENOMEM);
    }
}

void StreamSocket::checkOutgoingData(unsigned int events)
{
    if ((fd) &&
        (events & EVENT_OUT))
    {
        trySend();
    }
}

void StreamSocket::checkConnectionClosed(unsigned int events)
{
    if ((fd) &&
        (events & (EVENT_HUP | EVENT_ERR)))
    {
        const int error((events & EVENT_ERR) ? system.getSocketError(fd) : 0);
        close(error);
    }
}

void StreamSocket::allocateReceiveBuffer()
{
    static const size_t MIN_SIZE(1024 * 10);
    static const size_t ALLOC_SIZE(1024 * 20);

    if (input.getFreeSize() < MIN_SIZE)
    {
        input.allocate(ALLOC_SIZE);
    }
}

void StreamSocket::receive()
{
    const StreamBuffer::IOVecArray v(input.getFreeIOVec());
    struct msghdr msghdr = { nullptr, 0U, v.first, v.second };
    const ssize_t ssize(system.recvmsg(fd, &msghdr, 0));
    if (ssize == 0)
    {
        close(0);
    }
    else if (ssize > 0)
    {
        input.increaseUsed(static_cast<size_t>(ssize));
        dataReceivedCb();
    }
}

void StreamSocket::trySend()
{
    try
    {
        send();
    }
    catch (const SystemException &e)
    {
        close(e.error);
    }
    catch (const std::bad_alloc &)
    {
        close(ENOMEM);
    }
}

void StreamSocket::send()
{
    const StreamBuffer::IOVecArray v(output.getUsedIOVec());
    const struct msghdr msghdr = { nullptr, 0U, v.first, v.second };
    const ssize_t ssize(system.sendmsg(fd, &msghdr, MSG_NOSIGNAL));
    if (ssize > 0)
    {
        output.cutHead(static_cast<size_t>(ssize));
    }
    if (output.getUsedSize() > 0)
    {
        runner.modifyFDListener(fd, EVENT_IN | EVENT_OUT);
        sending = true;
    }
    else
    {
        runner.modifyFDListener(fd, EVENT_IN);
        sending = false;
        if (allDataSentCb)
        {
            allDataSentCb();
        }
    }
}

void StreamSocket::close(int error)
{
    if (fd)
    {
        runner.delFDListener(fd);
        fd.close();
        connectionClosedCb(error);
    }
}

void StreamSocket::startSending()
{
    if ((!sending) &&
        (!connecting) &&
        (output.getUsedSize() > 0))
    {
        trySend();
    }
}

void StreamSocket::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, system);
    DUMP_ADDRESS_OF(os, runner);
    DUMP_VALUE_OF(os, fd);
    DUMP_VALUE_OF(os, connecting);
    DUMP_VALUE_OF(os, sending);
    DUMP_VALUE_OF(os, input);
    DUMP_VALUE_OF(os, output);
    DUMP_VALUE_OF(os, connectionEstablishedCb);
    DUMP_VALUE_OF(os, connectionClosedCb);
    DUMP_VALUE_OF(os, dataReceivedCb);
    DUMP_VALUE_OF(os, allDataSentCb);
    DUMP_RESULT_OF(os, getName());
    DUMP_RESULT_OF(os, getPeer());
    DUMP_AS(os, FDListener);
    DUMP_END(os);
}
