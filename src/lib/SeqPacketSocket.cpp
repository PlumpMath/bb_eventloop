#include <EventLoop/SeqPacketSocket.hpp>

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
    const int TYPE(SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC);
}

SeqPacketSocket::SeqPacketSocket(Runner                         &runner,
                                 int                            protocol,
                                 size_t                         maxSize,
                                 const SocketAddress            &local,
                                 const SocketAddress            &remote,
                                 const ConnectionEstablishedCb  &connectionEstablishedCb,
                                 const ConnectionClosedCb       &connectionClosedCb,
                                 const DataReceivedCb           &dataReceivedCb,
                                 const AllDataSentCb            &allDataSentCb):
    SeqPacketSocket(System::getSystem(), runner, protocol, maxSize, local, remote, connectionEstablishedCb, connectionClosedCb, dataReceivedCb, allDataSentCb) { }

SeqPacketSocket::SeqPacketSocket(Runner                         &runner,
                                 int                            protocol,
                                 size_t                         maxSize,
                                 const SocketAddress            &remote,
                                 const ConnectionEstablishedCb  &connectionEstablishedCb,
                                 const ConnectionClosedCb       &connectionClosedCb,
                                 const DataReceivedCb           &dataReceivedCb,
                                 const AllDataSentCb            &allDataSentCb):
    SeqPacketSocket(System::getSystem(), runner, protocol, maxSize, remote, connectionEstablishedCb, connectionClosedCb, dataReceivedCb, allDataSentCb) { }

SeqPacketSocket::SeqPacketSocket(Runner                     &runner,
                                 FileDescriptor             &fd,
                                 size_t                     maxSize,
                                 const ConnectionClosedCb   &connectionClosedCb,
                                 const DataReceivedCb       &dataReceivedCb,
                                 const AllDataSentCb        &allDataSentCb):
    SeqPacketSocket(System::getSystem(), runner, fd, maxSize, connectionClosedCb, dataReceivedCb, allDataSentCb) { }

SeqPacketSocket::SeqPacketSocket(System                         &system,
                                 Runner                         &runner,
                                 int                            protocol,
                                 size_t                         maxSize,
                                 const SocketAddress            &local,
                                 const SocketAddress            &remote,
                                 const ConnectionEstablishedCb  &connectionEstablishedCb,
                                 const ConnectionClosedCb       &connectionClosedCb,
                                 const DataReceivedCb           &dataReceivedCb,
                                 const AllDataSentCb            &allDataSentCb):
    system(system),
    runner(runner),
    fd(system, system.socket(remote.getFamily(), TYPE, protocol)),
    maxSize(maxSize),
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

SeqPacketSocket::SeqPacketSocket(System                         &system,
                                 Runner                         &runner,
                                 int                            protocol,
                                 size_t                         maxSize,
                                 const SocketAddress            &remote,
                                 const ConnectionEstablishedCb  &connectionEstablishedCb,
                                 const ConnectionClosedCb       &connectionClosedCb,
                                 const DataReceivedCb           &dataReceivedCb,
                                 const AllDataSentCb            &allDataSentCb):
    system(system),
    runner(runner),
    fd(system, system.socket(remote.getFamily(), TYPE, protocol)),
    maxSize(maxSize),
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

SeqPacketSocket::SeqPacketSocket(System                     &system,
                                 Runner                     &runner,
                                 FileDescriptor             &fd,
                                 size_t                     maxSize,
                                 const ConnectionClosedCb   &connectionClosedCb,
                                 const DataReceivedCb       &dataReceivedCb,
                                 const AllDataSentCb        &allDataSentCb):
    system(system),
    runner(runner),
    fd(system, fd.release()),
    maxSize(maxSize),
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

SeqPacketSocket::~SeqPacketSocket()
{
    if (fd)
    {
        runner.delFDListener(fd);
    }
}

SocketAddress SeqPacketSocket::getName() const
{
    SocketAddress sa;
    system.getSockName(fd, sa.getPointer(), sa.getSizePointer());
    return sa;
}

SocketAddress SeqPacketSocket::getPeer() const
{
    SocketAddress sa;
    system.getPeerName(fd, sa.getPointer(), sa.getSizePointer());
    return sa;
}

void SeqPacketSocket::send(const void                                   *data,
                           size_t                                       size,
                           const std::shared_ptr<const ControlMessage>  &cm)
{
    if ((sending) ||
        (connecting) ||
        (!trySendFast(data, size, *cm)))
    {
        if (fd)
        {
            queue(data, size, cm);
        }
    }
    else if (allDataSentCb)
    {
        allDataSentCb();
    }
}

void SeqPacketSocket::send(const void   *data,
                           size_t       size)
{
    static const std::shared_ptr<const ControlMessage> dummy(std::make_shared<ControlMessage>());
    send(data, size, dummy);
}

void SeqPacketSocket::handleEventsCb(int,
                                     unsigned int   events)
{
    checkConnectionEstablished(events);
    checkIncomingData(events);
    checkOutgoingData(events);
    checkConnectionClosed(events);
}

void SeqPacketSocket::checkConnectionEstablished(unsigned int events)
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
        if (!sending)
        {
            runner.modifyFDListener(fd, EVENT_IN);
        }
    }
}

void SeqPacketSocket::checkIncomingData(unsigned int events)
{
    try
    {
        if (events & EVENT_IN)
        {
            receive();
        }
    }
    catch (const SystemException &e)
    {
        close(e.error);
    }
}

void SeqPacketSocket::checkOutgoingData(unsigned int events)
{
    if ((fd) &&
        (events & EVENT_OUT))
    {
        sendSlow();
    }
}

void SeqPacketSocket::allocateReceiveBuffers()
{
    static const size_t MAX_CM_SIZE(1024);
    if (buffer.empty())
    {
        buffer.resize(maxSize, 0);
    }
    if (cm.capacity() < MAX_CM_SIZE)
    {
        cm.reserve(MAX_CM_SIZE);
    }
}

void SeqPacketSocket::receive()
{
    allocateReceiveBuffers();
    struct iovec iovec[1] = { { buffer.data(), buffer.size() } };
    struct msghdr msghdr =
    {
        nullptr,
        0,
        iovec,
        sizeof(iovec) / sizeof(iovec[0]),
        cm.data(),
        cm.capacity(),
        0
    };
    const ssize_t ssize(system.recvmsg(fd, &msghdr, 0));
    if (ssize >= 0)
    {
        cm.markUsed(msghdr.msg_controllen);
        dataReceivedCb(buffer.data(), ssize, cm);
    }
}

void SeqPacketSocket::checkConnectionClosed(unsigned int events)
{
    if ((fd) &&
        (events & (EVENT_HUP | EVENT_ERR)))
    {
        const int error((events & EVENT_ERR) ? system.getSocketError(fd) : 0);
        close(error);
    }
}

void SeqPacketSocket::sendSlow()
{
    while (!outputQueue.empty())
    {
        if (sendData(outputQueue.front()))
        {
            outputQueue.pop_front();
        }
        else
        {
            return;
        }
    }
    sending = false;
    runner.modifyFDListener(fd, EVENT_IN);
    if (allDataSentCb)
    {
        allDataSentCb();
    }
}

bool SeqPacketSocket::sendData(const Data &data)
{
    return trySendFast(data.buffer->data(), data.buffer->size(), *data.cm);
}

bool SeqPacketSocket::trySendFast(const void            *data,
                                  size_t                size,
                                  const ControlMessage  &cm)
{
    try
    {
        return sendFast(data, size, cm);
    }
    catch (const SystemException &e)
    {
        close(e.error);
        return false;
    }
}

bool SeqPacketSocket::sendFast(const void           *data,
                               size_t               size,
                               const ControlMessage &cm)
{
    struct iovec iovec[1] = { { const_cast<void *>(data), size } };
    const struct msghdr msghdr =
    {
        nullptr,
        0,
        iovec,
        sizeof(iovec) / sizeof(iovec[0]),
        const_cast<void *>(cm.data()),
        cm.size(),
        0
    };
    return (system.sendmsg(fd, &msghdr, MSG_NOSIGNAL) > 0);
}

void SeqPacketSocket::queue(const void                                  *data,
                            size_t                                      size,
                            const std::shared_ptr<const ControlMessage> &cm)
{
    sending = true;
    outputQueue.push_back(Data(data, size, cm));
    runner.modifyFDListener(fd, EVENT_IN | EVENT_OUT);
}

void SeqPacketSocket::close(int error)
{
    if (fd)
    {
        runner.delFDListener(fd);
        fd.close();
        connectionClosedCb(error);
    }
}

void SeqPacketSocket::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, system);
    DUMP_ADDRESS_OF(os, runner);
    DUMP_VALUE_OF(os, fd);
    DUMP_VALUE_OF(os, maxSize);
    DUMP_VALUE_OF(os, connecting);
    DUMP_VALUE_OF(os, sending);
    DUMP_RESULT_OF(os, buffer.size());
    DUMP_RESULT_OF(os, cm.capacity());
    DUMP_CONTAINER(os, outputQueue);
    DUMP_VALUE_OF(os, connectionEstablishedCb);
    DUMP_VALUE_OF(os, connectionClosedCb);
    DUMP_VALUE_OF(os, dataReceivedCb);
    DUMP_VALUE_OF(os, allDataSentCb);
    DUMP_RESULT_OF(os, getName());
    DUMP_RESULT_OF(os, getPeer());
    DUMP_AS(os, FDListener);
    DUMP_END(os);
}

SeqPacketSocket::Data::Data(const void                                  *data,
                            size_t                                      size,
                            const std::shared_ptr<const ControlMessage> &cm):
    cm(cm)
{
    const char *begin(static_cast<const char *>(data));
    const char *end(begin + size);
    buffer = std::make_shared<std::vector<char>>(begin, end);
}

void SeqPacketSocket::Data::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_RESULT_OF(os, buffer->data());
    DUMP_RESULT_OF(os, buffer->size());
    DUMP_RESULT_OF(os, cm->data());
    DUMP_RESULT_OF(os, cm->size());
    DUMP_END(os);
}
