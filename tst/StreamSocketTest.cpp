#include <cstring>
#include <cerrno>
#include <gtest/gtest.h>

#include <EventLoop/SocketAddress.hpp>
#include <EventLoop/SystemException.hpp>

#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/DummyFDListenerMap.hpp"
#include "private/tst/StreamSocketMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

TEST(StreamSocket, constructorCreatesSocketAndBindsAndConnectsAndAddsToMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP))
        .Times(1)
        .WillOnce(Return(4));
    EXPECT_CALL(sm, bind(4, _, _))
        .Times(1);
    EXPECT_CALL(sm, connect(4, _, _))
        .Times(1);
    StreamSocketMock ss(sm, flm, IPPROTO_TCP, SocketAddress::createIPv4("0.0.0.0", "0"), SocketAddress::createIPv4("localhost", "ssh"));
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(4));
}

TEST(StreamSocket, constructorCreatesSocketAndConnectsAndAddsToMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(PF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0))
        .Times(1)
        .WillOnce(Return(5));
    EXPECT_CALL(sm, connect(5, _, _))
        .Times(1);
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(5));
}

TEST(StreamSocket, fdConstructorCreatesSocketAndSetsCloseOnExecAndNonBlockAndAddsToMapWithOnlyInEvent)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, setCloseOnExec(5))
        .Times(1);
    EXPECT_CALL(sm, setNonBlock(5))
        .Times(1);
    FileDescriptor fd(5);
    StreamSocketMock ss(sm, flm, fd);
    EXPECT_FALSE(fd);
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(5));
}

TEST(StreamSocket, destructorClosesTheFDAndRemovesFromMap)
{
    DummyFDListenerMap flm;
    SystemMock sm;
    {
        EXPECT_CALL(sm, socket(_, _, _))
            .Times(1)
            .WillOnce(Return(6));
        StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
        EXPECT_CALL(sm, close(6))
            .Times(1);
    }
    EXPECT_THROW(flm.get(6), Runner::NonExistingFDListener);
}

TEST(StreamSocket, getNameCallsSystemGetSockName)
{
    DummyFDListenerMap flm;
    SystemMock sm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(6));
    const SocketAddress sa(SocketAddress::createUnix("/var/run/socket"));
    StreamSocketMock ss(sm, flm, 0, sa);
    EXPECT_CALL(sm, getSockName(6, NotNull(), Pointee(SocketAddress().getAvailableSize())))
        .Times(1)
        .WillOnce(Invoke([&sa] (int, struct sockaddr *addr, socklen_t *len) { memcpy(addr, sa.getPointer(), sa.getUsedSize()); *len = sa.getUsedSize(); }));
    EXPECT_EQ(sa, ss.getName());
}

TEST(StreamSocket, getPeerCallsSystemGetPeerName)
{
    DummyFDListenerMap flm;
    SystemMock sm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(6));
    const SocketAddress sa(SocketAddress::createUnix("/var/run/socket"));
    StreamSocketMock ss(sm, flm, 0, sa);
    EXPECT_CALL(sm, getPeerName(6, NotNull(), Pointee(SocketAddress().getAvailableSize())))
        .Times(1)
        .WillOnce(Invoke([&sa] (int, struct sockaddr *addr, socklen_t *len) { memcpy(addr, sa.getPointer(), sa.getUsedSize()); *len = sa.getUsedSize(); }));
    EXPECT_EQ(sa, ss.getPeer());
}

TEST(StreamSocket, firstInOrOutEventMeansConnectionIsEstablishedAndOutEventIsRemovedIfNoOutGoingData)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(111));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(ss, connectionEstablishedCb())
        .Times(1);
    ss.handleEvents(111, FDListener::EVENT_OUT);
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(111));
    /* And no more connectionEstablishedCb callbacks after the first event */
    ss.handleEvents(111, FDListener::EVENT_OUT);
}

TEST(StreamSocket, firstInOrOutEventMeansConnectionIsEstablishedAndOutEventIsNotRemovedIfOutGoingData)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(111));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    ss.getOutput().appendData("blaa");
    EXPECT_CALL(ss, connectionEstablishedCb())
        .Times(1);
    ss.handleEvents(111, FDListener::EVENT_OUT);
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(111));
    /* And no more connectionEstablishedCb callbacks after the first event */
    ss.handleEvents(111, FDListener::EVENT_OUT);
}

TEST(StreamSocket, hupOrErrEventMeansThatConnectionIsNotEstablished)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(111));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(ss, connectionEstablishedCb())
        .Times(0);
    ss.handleEvents(111, FDListener::EVENT_OUT | FDListener::EVENT_HUP);
}

TEST(StreamSocket, hupEventClosesTheSocketAndRemovesFromMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, getSocketError(7))
        .Times(0);
    EXPECT_CALL(sm, close(7))
        .Times(1);
    EXPECT_CALL(ss, connectionClosedCb(0))
        .Times(1);
    ss.handleEvents(7, FDListener::EVENT_HUP);
    EXPECT_THROW(flm.get(7), Runner::NonExistingFDListener);
}

TEST(StreamSocket, errEventClosesTheSocketAndRemovesFromMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
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

TEST(StreamSocket, inEventAndReadZeroClosesTheSocketAndRemovesFromMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, recvmsg(7, _, 0))
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(sm, close(7))
        .Times(1);
    ss.handleEvents(7, FDListener::EVENT_IN);
    EXPECT_THROW(flm.get(7), Runner::NonExistingFDListener);
}

TEST(StreamSocket, socketIsClosedOnlyOnceEvenIfInAndHupAndErrEventsAndReadReturnsZero)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, recvmsg(7, _, 0))
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(sm, getSocketError(_))
        .Times(0);
    EXPECT_CALL(sm, close(7))
        .Times(1);
    ss.handleEvents(7, FDListener::EVENT_IN | FDListener::EVENT_HUP | FDListener::EVENT_ERR);
    EXPECT_THROW(flm.get(7), Runner::NonExistingFDListener);
}

TEST(StreamSocket, inEventReadsFromSocketAndCallsDataReceivedCb)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, recvmsg(7, _, 0))
        .Times(1)
        .WillOnce(Return(10));
    EXPECT_CALL(ss, dataReceivedCb());
    ss.handleEvents(7, FDListener::EVENT_IN);
    EXPECT_EQ(10U, ss.getInput().getUsedSize());
}

TEST(StreamSocket, systemExceptionFromReadClosesTheSocketAndRemovesFromMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
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

TEST(StreamSocket, badAllocFromReadClosesTheSocketAndRemovesFromMap)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(7));
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    EXPECT_CALL(sm, recvmsg(7, _, _))
        .Times(1)
        .WillOnce(Throw(std::bad_alloc()));
    EXPECT_CALL(sm, close(7))
        .Times(1);
    EXPECT_CALL(ss, connectionClosedCb(ENOMEM))
        .Times(1);
    ss.handleEvents(7, FDListener::EVENT_IN);
    EXPECT_THROW(flm.get(7), Runner::NonExistingFDListener);
}

TEST(StreamSocket, startSendingWhileConnectingDoesNothing)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    StreamSocketMock ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket"));
    ss.getOutput().appendData("blaa");
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(0);
    ss.startSending();
}

namespace
{
    /* Helper for cases where we need 'connection established' socket */
    class ConnectedStreamSocket: public testing::Test
    {
    public:
        SystemMock sm;
        DummyFDListenerMap flm;
        StreamSocketMock ss;

        ConnectedStreamSocket(): ss(sm, flm, 0, SocketAddress::createUnix("/var/run/socket")) { }

    protected:
        virtual void SetUp()
        {
            ss.handleEvents(0, FDListener::EVENT_OUT);
        }
    };
}

TEST_F(ConnectedStreamSocket, startSendingWithEmptyOutputDoesNothing)
{
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(0);
    ss.startSending();
}

TEST_F(ConnectedStreamSocket, startSendingCallsSendMsg)
{
    ss.getOutput().appendData("blaa");
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(1);
    ss.startSending();
}

TEST_F(ConnectedStreamSocket, partialDataSentOutEventIsAdded)
{
    ss.getOutput().appendData("1234");
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Return(3));
    EXPECT_CALL(ss, allDataSentCb())
        .Times(0);
    ss.startSending();
    EXPECT_EQ(1U, ss.getOutput().getUsedSize());
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(0));
}

TEST_F(ConnectedStreamSocket, ifNothingIsSentOutEventIsAdded)
{
    ss.getOutput().appendData("1234");
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Return(-1));
    EXPECT_CALL(ss, allDataSentCb())
        .Times(0);
    ss.startSending();
    EXPECT_EQ(4U, ss.getOutput().getUsedSize());
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(0));
}

TEST_F(ConnectedStreamSocket, allDataSentOutEventIsRemovedAndCallbackIsCalled)
{
    ss.getOutput().appendData("12345678");
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(2)
        .WillOnce(Return(4))
        .WillOnce(Return(4));
    ss.startSending();
    EXPECT_CALL(ss, allDataSentCb())
        .Times(1);
    ss.handleEvents(0, FDListener::EVENT_OUT);
    EXPECT_EQ(0U, ss.getOutput().getUsedSize());
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(0));
}

TEST_F(ConnectedStreamSocket, systemExceptionFromSendMsgClosesTheFDAndRemovesFromMap)
{
    ss.getOutput().appendData("12345678");
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Throw(SystemException("sendmsg", ECONNRESET)));
    EXPECT_CALL(sm, close(_))
        .Times(1);
    EXPECT_CALL(ss, connectionClosedCb(ECONNRESET))
        .Times(1);
    ss.startSending();
    EXPECT_THROW(flm.get(0), Runner::NonExistingFDListener);
}

TEST_F(ConnectedStreamSocket, badAllocFromSendMsgClosesTheFDAndRemovesFromMap)
{
    ss.getOutput().appendData("12345678");
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Throw(std::bad_alloc()));
    EXPECT_CALL(sm, close(_))
        .Times(1);
    EXPECT_CALL(ss, connectionClosedCb(ENOMEM))
        .Times(1);
    ss.startSending();
    EXPECT_THROW(flm.get(0), Runner::NonExistingFDListener);
}

TEST_F(ConnectedStreamSocket, startSendingWhenAlreadySendingDoesNothing)
{
    ss.getOutput().appendData("12345678");
    EXPECT_CALL(sm, sendmsg(_, _, MSG_NOSIGNAL))
        .Times(1)
        .WillOnce(Return(1));
    EXPECT_CALL(ss, allDataSentCb())
        .Times(0);
    ss.startSending();
    ss.startSending();
}

TEST_F(ConnectedStreamSocket, ifFDIsClosedWhileCheckingEventInOutputEventIsNotChecked)
{
    ss.getOutput().appendData(1);
    EXPECT_CALL(sm, recvmsg(_, _, _))
        .Times(1)
        .WillOnce(Return(0));
    ss.handleEvents(111, FDListener::EVENT_IN | FDListener::EVENT_OUT);
}

TEST(StreamSocket, exceptionInTheMiddleOfConstructorClosesSocket)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    EXPECT_CALL(sm, socket(PF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0))
        .Times(1)
        .WillOnce(Return(5));
    EXPECT_CALL(sm, connect(5, _, _))
        .Times(1)
        .WillOnce(Throw(SystemException("connect", ECONNREFUSED)));
    EXPECT_CALL(sm, close(5))
        .Times(1);
    EXPECT_THROW(StreamSocketMock(sm, flm, 0, SocketAddress::createUnix("/var/run/socket")), SystemException);
    EXPECT_THROW(flm.get(5), Runner::NonExistingFDListener);
}

TEST(StreamSocket, dump)
{
    SystemMock sm;
    DummyFDListenerMap flm;
    StreamSocketMock ss(sm, flm, IPPROTO_TCP, SocketAddress::createIPv4("localhost", "ssh"));
    EXPECT_VALID_DUMP(ss);
}
