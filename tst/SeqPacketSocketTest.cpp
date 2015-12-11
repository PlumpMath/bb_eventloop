#include <cstring>
#include <cerrno>
#include <gtest/gtest.h>

#include <EventLoop/SocketAddress.hpp>
#include <EventLoop/SystemException.hpp>

#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/DummyFDListenerMap.hpp"
#include "private/tst/SeqPacketSocketMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

TEST(SeqPacketSocket, constructorCreatesSocketAndBindsAndConnectsAndAddsToMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(PF_INET, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 123))
        .Times(1)
        .WillOnce(Return(4));
    EXPECT_CALL(sm, bind(4, _, _))
        .Times(1);
    EXPECT_CALL(sm, connect(4, _, _))
        .Times(1);
    SeqPacketSocketMock ss(sm, flm, 123, SocketAddress::createIPv4("0.0.0.0", "0"), SocketAddress::createIPv4("localhost", "ssh"));
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(4));
}

TEST(SeqPacketSocket, constructorCreatesSocketAndConnectsAndAddsToMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(PF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 123))
        .Times(1)
        .WillOnce(Return(5));
    EXPECT_CALL(sm, connect(5, _, _))
        .Times(1);
    SeqPacketSocketMock ss(sm, flm, 123, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(5));
}

TEST(SeqPacketSocket, fdConstructorCreatesSocketAndSetsCloseOnExecAndNonBlockAndAddsToMapWithOnlyInEvent)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, setCloseOnExec(5))
        .Times(1);
    EXPECT_CALL(sm, setNonBlock(5))
        .Times(1);
    FileDescriptor fd(5);
    SeqPacketSocketMock ss(sm, flm, fd);
    EXPECT_FALSE(fd);
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(5));
}

TEST(SeqPacketSocket, destructorClosesTheFDAndRemovesFromMap)
{
    DummyFDListenerMap flm;
    SystemMock sm;
    {
        EXPECT_CALL(sm, socket(_, _, _))
            .Times(1)
            .WillOnce(Return(6));
        SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
        EXPECT_CALL(sm, close(6))
            .Times(1);
    }
    EXPECT_THROW(flm.get(6), Runner::NonExistingFDListener);
}

TEST(SeqPacketSocket, getNameCallsSystemGetSockName)
{
    DummyFDListenerMap flm;
    SystemMock sm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(6));
    const SocketAddress sa(SocketAddress::createUnix("/var/run/socket"));
    SeqPacketSocketMock ss(sm, flm, 0, sa);
    EXPECT_CALL(sm, getSockName(6, NotNull(), Pointee(SocketAddress().getAvailableSize())))
        .Times(1)
        .WillOnce(Invoke([&sa] (int, struct sockaddr *addr, socklen_t *len) { memcpy(addr, sa.getPointer(), sa.getUsedSize()); *len = sa.getUsedSize(); }));
    EXPECT_EQ(sa, ss.getName());
}

TEST(SeqPacketSocket, getPeerCallsSystemGetPeerName)
{
    DummyFDListenerMap flm;
    SystemMock sm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(6));
    const SocketAddress sa(SocketAddress::createUnix("/var/run/socket"));
    SeqPacketSocketMock ss(sm, flm, 0, sa);
    EXPECT_CALL(sm, getPeerName(6, NotNull(), Pointee(SocketAddress().getAvailableSize())))
        .Times(1)
        .WillOnce(Invoke([&sa] (int, struct sockaddr *addr, socklen_t *len) { memcpy(addr, sa.getPointer(), sa.getUsedSize()); *len = sa.getUsedSize(); }));
    EXPECT_EQ(sa, ss.getPeer());
}

TEST(SeqPacketSocket, firstInOrOutEventMeansConnectionIsEstablishedAndOutEventIsRemovedIfNoOutGoingData)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(111));
    SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(ss, connectionEstablishedCb())
        .Times(1);
    ss.handleEvents(111, FDListener::EVENT_OUT);
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(111));
    /* And no more connectionEstablishedCb callbacks after the first event */
    ss.handleEvents(111, FDListener::EVENT_OUT);
}

TEST(SeqPacketSocket, firstInOrOutEventMeansConnectionIsEstablishedAndOutEventIsNotRemovedIfOutGoingData)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(111));
    SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    static const int data(123);
    ss.send(&data, sizeof(data));
    EXPECT_CALL(ss, connectionEstablishedCb())
        .Times(1);
    ss.handleEvents(111, FDListener::EVENT_OUT);
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(111));
    /* And no more connectionEstablishedCb callbacks after the first event */
    ss.handleEvents(111, FDListener::EVENT_OUT);
}

TEST(SeqPacketSocket, hupOrErrEventMeansThatConnectionIsNotEstablished)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(111));
    SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(ss, connectionEstablishedCb())
        .Times(0);
    ss.handleEvents(111, FDListener::EVENT_OUT | FDListener::EVENT_HUP);
}

TEST(SeqPacketSocket, hupEventClosesTheSocketAndRemovesFromMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, getSocketError(7))
        .Times(0);
    EXPECT_CALL(sm, close(7))
        .Times(1);
    EXPECT_CALL(ss, connectionClosedCb(0))
        .Times(1);
    ss.handleEvents(7, FDListener::EVENT_HUP);
    EXPECT_THROW(flm.get(7), Runner::NonExistingFDListener);
}

TEST(SeqPacketSocket, errEventClosesTheSocketAndRemovesFromMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, getSocketError(7))
        .Times(1)
        .WillOnce(Return(ECONNRESET));
    EXPECT_CALL(sm, close(7))
        .Times(1);
    EXPECT_CALL(ss, connectionClosedCb(ECONNRESET))
        .Times(1);
    ss.handleEvents(7, FDListener::EVENT_ERR);
    EXPECT_THROW(flm.get(7), Runner::NonExistingFDListener);
}

TEST(SeqPacketSocket, inEventReadsFromSocketAndCallsDataReceivedCb)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, recvmsg(7, Ne(nullptr), 0))
        .Times(1)
        .WillOnce(Invoke([] (int, struct msghdr *msg, int) -> ssize_t
                         {
                             EXPECT_EQ(nullptr, msg->msg_name);
                             EXPECT_EQ(0U, msg->msg_namelen);
                             EXPECT_NE(nullptr, msg->msg_iov);
                             EXPECT_EQ(1U, msg->msg_iovlen);
                             EXPECT_NE(nullptr, msg->msg_iov[0].iov_base);
                             EXPECT_EQ(SeqPacketSocketMock::MAX_SIZE, msg->msg_iov[0].iov_len);
                             EXPECT_NE(nullptr, msg->msg_control);
                             EXPECT_LE(1000U, msg->msg_controllen);
                             EXPECT_EQ(0, msg->msg_flags);
                             return 10;
                         }));
    EXPECT_CALL(ss, dataReceivedCb(_, 10U, _));
    ss.handleEvents(7, FDListener::EVENT_IN);
}

TEST(SeqPacketSocket, systemExceptionFromReadClosesTheSocketAndRemovesFromMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, recvmsg(7, _, _))
        .Times(1)
        .WillOnce(Throw(SystemException("recvmsg", ECONNRESET)));
    EXPECT_CALL(sm, close(7))
        .Times(1);
    EXPECT_CALL(ss, connectionClosedCb(ECONNRESET))
        .Times(1);
    ss.handleEvents(7, FDListener::EVENT_IN);
    EXPECT_THROW(flm.get(7), Runner::NonExistingFDListener);
}

TEST(SeqPacketSocket, sendWhileConnectingDoesNothing)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, sendmsg(_, _, _))
        .Times(0);
    static const int data(123);
    ss.send(&data, sizeof(data));
}

namespace
{
    /* Helper for cases where we need 'connection established' socket */
    class ConnectedSeqPacketSocket: public testing::Test
    {
    public:
        int data;
        std::shared_ptr<ControlMessage> cm;
        SystemMock sm;
        DummyFDListenerMap flm;
        SeqPacketSocketMock ss;

        ConnectedSeqPacketSocket():
            data(123),
            cm(std::make_shared<ControlMessage>(1, 2, 3)),
            ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket")) { }

    protected:
        virtual void SetUp()
        {
            ss.handleEvents(0, FDListener::EVENT_OUT);
        }
    };
}

TEST_F(ConnectedSeqPacketSocket, sendWithControlMessageCallsSendMsg)
{
    InSequence dummy;
    EXPECT_CALL(sm, sendmsg(_, Ne(nullptr), MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Invoke([this] (int, const struct msghdr *msg, int) -> ssize_t
                         {
                             EXPECT_EQ(nullptr, msg->msg_name);
                             EXPECT_EQ(0U, msg->msg_namelen);
                             EXPECT_NE(nullptr, msg->msg_iov);
                             EXPECT_EQ(1U, msg->msg_iovlen);
                             EXPECT_NE(nullptr, msg->msg_iov[0].iov_base);
                             EXPECT_EQ(sizeof(data), msg->msg_iov[0].iov_len);
                             EXPECT_NE(nullptr, msg->msg_control);
                             EXPECT_LE(cm->size(), msg->msg_controllen);
                             EXPECT_EQ(0, msg->msg_flags);
                             return sizeof(data);
                         }));
    EXPECT_CALL(ss, allDataSentCb())
        .Times(1);
    ss.send(&data, sizeof(data), cm);
}

TEST_F(ConnectedSeqPacketSocket, sendWithoutControlMessageCallsSendMsg)
{
    EXPECT_CALL(sm, sendmsg(_, Ne(nullptr), MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Invoke([this] (int, const struct msghdr *msg, int) -> ssize_t
                         {
                             EXPECT_EQ(nullptr, msg->msg_name);
                             EXPECT_EQ(0U, msg->msg_namelen);
                             EXPECT_NE(nullptr, msg->msg_iov);
                             EXPECT_EQ(1U, msg->msg_iovlen);
                             EXPECT_NE(nullptr, msg->msg_iov[0].iov_base);
                             EXPECT_EQ(sizeof(data), msg->msg_iov[0].iov_len);
                             EXPECT_EQ(nullptr, msg->msg_control);
                             EXPECT_EQ(0U, msg->msg_controllen);
                             EXPECT_EQ(0, msg->msg_flags);
                             return sizeof(data);
                         }));
    ss.send(&data, sizeof(data));
}

TEST_F(ConnectedSeqPacketSocket, eagainWhileSendingIsQueued)
{
    InSequence dummy;
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Return(-1));
    EXPECT_CALL(ss, allDataSentCb())
        .Times(0);
    ss.send(&data, sizeof(data));
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(0));
    Mock::VerifyAndClearExpectations(&sm);
    /* Next send goes directly to queue */
    EXPECT_CALL(sm, sendmsg(_, _, _))
        .Times(0);
    ss.send(&data, sizeof(data));
}

TEST_F(ConnectedSeqPacketSocket, allDataSentOutEventIsRemovedAndCallbackIsCalled)
{
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(3)
        .WillOnce(Return(-1))
        .WillOnce(Return(sizeof(data)))
        .WillOnce(Return(sizeof(data)));
    ss.send(&data, sizeof(data));
    ss.send(&data, sizeof(data));
    EXPECT_CALL(ss, allDataSentCb())
        .Times(1);
    ss.handleEvents(0, FDListener::EVENT_OUT);
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(0));
}

TEST_F(ConnectedSeqPacketSocket, systemExceptionFromSendMsgClosesTheFDAndRemovesFromMap)
{
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Throw(SystemException("sendmsg", ECONNRESET)));
    EXPECT_CALL(sm, close(_))
        .Times(1);
    EXPECT_CALL(ss, connectionClosedCb(ECONNRESET))
        .Times(1);
    ss.send(&data, sizeof(data));
    EXPECT_THROW(flm.get(0), Runner::NonExistingFDListener);
}

TEST_F(ConnectedSeqPacketSocket, sendWhenAlreadySendingDoesNothing)
{
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Return(-1));
    ss.send(&data, sizeof(data));
    EXPECT_CALL(ss, allDataSentCb())
        .Times(0);
    ss.send(&data, sizeof(data));
    ss.send(&data, sizeof(data));
}

TEST(SeqPacketSocket, exceptionInTheMiddleOfConstructorClosesSocket)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(PF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 33))
        .Times(1)
        .WillOnce(Return(5));
    EXPECT_CALL(sm, connect(5, _, _))
        .Times(1)
        .WillOnce(Throw(SystemException("connect", ECONNREFUSED)));
    EXPECT_CALL(sm, close(5))
        .Times(1);
    EXPECT_THROW(SeqPacketSocketMock(sm, flm, 33, SocketAddress::createUnix("/var/run/socket")), SystemException);
    EXPECT_THROW(flm.get(5), Runner::NonExistingFDListener);
}

TEST(SeqPacketSocket, dump)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    SeqPacketSocketMock ss(sm, flm, 0, SocketAddress::createIPv4("localhost", "ssh"));
    EXPECT_VALID_DUMP(ss);
}
