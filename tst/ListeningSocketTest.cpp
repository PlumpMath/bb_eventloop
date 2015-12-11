#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/SocketAddress.hpp>

#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/DummyFDListenerMap.hpp"
#include "private/tst/ListeningSocketMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

TEST(ListeningSocket, constructorCreatesSocketAndSetsCloseOnExecAndNonBlockAndReuseAddrAndBindsAndListensAndAddsToMap)
{
    InSequence dummy;
    DummyFDListenerMap flm;
    SystemMock sm;
    EXPECT_CALL(sm, socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP))
        .Times(1)
        .WillOnce(Return(123));
    EXPECT_CALL(sm, setReuseAddr(123))
        .Times(1);
    EXPECT_CALL(sm, bind(123, _, _))
        .Times(1);
    EXPECT_CALL(sm, listen(123, _))
        .Times(1);
    ListeningSocketMock ls(sm, flm, SOCK_STREAM, IPPROTO_TCP, SocketAddress::createIPv4("10.10.10.10", "10"), ListeningSocket::NewAcceptedSocketCb());
    flm.get(123);
}

TEST(ListeningSocket, constructorWithFDSetsCloseOnExecAndNonBlockAndTakesOwnershipOfTheFDAndAddsToMap)
{
    InSequence dummy;
    DummyFDListenerMap flm;
    SystemMock sm;
    FileDescriptor fd(open("/dev/null", O_RDONLY));
    const int value(static_cast<int>(fd));
    EXPECT_CALL(sm, setCloseOnExec(value))
        .Times(1);
    EXPECT_CALL(sm, setNonBlock(value))
        .Times(1);
    ListeningSocketMock ls(sm, flm, fd, ListeningSocket::NewAcceptedSocketCb());
    EXPECT_FALSE(static_cast<bool>(fd));
    flm.get(value);
}

TEST(ListeningSocket, destructorClosesTheFDAndRemovesFromMap)
{
    InSequence dummy;
    DummyFDListenerMap flm;
    SystemMock sm;
    {
        EXPECT_CALL(sm, socket(_, _, _))
            .Times(1)
            .WillOnce(Return(123));
        ListeningSocketMock ls(sm, flm, SOCK_STREAM, IPPROTO_TCP, SocketAddress::createIPv4("10.10.10.10", "10"), ListeningSocket::NewAcceptedSocketCb());
        EXPECT_CALL(sm, close(123))
            .Times(1);
    }
    EXPECT_THROW(flm.get(123), Runner::NonExistingFDListener);
}

TEST(ListeningSocket, getNameCallsSystemGetSockName)
{
    DummyFDListenerMap flm;
    SystemMock sm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(6));
    const SocketAddress sa(SocketAddress::createIPv4("10.10.10.10", "10"));
    ListeningSocketMock ls(sm, flm, SOCK_STREAM, IPPROTO_TCP, sa, ListeningSocket::NewAcceptedSocketCb());
    EXPECT_CALL(sm, getSockName(6, NotNull(), Pointee(SocketAddress().getAvailableSize())))
        .Times(1)
        .WillOnce(Invoke([&sa] (int, struct sockaddr *addr, socklen_t *len) { memcpy(addr, sa.getPointer(), sa.getUsedSize()); *len = sa.getUsedSize(); }));
    EXPECT_EQ(sa, ls.getName());
}

TEST(ListeningSocket, handleEventsCbAcceptsAllConnections)
{
    InSequence dummy;
    std::vector<int> fds;
    DummyFDListenerMap flm;
    SystemMock sm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(112));
    ListeningSocketMock ls(sm,
                           flm,
                           SOCK_STREAM,
                           IPPROTO_TCP,
                           SocketAddress::createIPv4("10.10.10.10", "10"),
                           [&fds] (FileDescriptor &fd, const SocketAddress &)
                           {
                               fds.push_back(fd.release());
                           });
    EXPECT_CALL(sm, accept(112, _, _))
        .Times(3)
        .WillOnce(Return(11))
        .WillOnce(Return(12))
        .WillOnce(Return(-1));
    ls.handleEvents(112, FDListener::EVENT_IN);
    EXPECT_THAT(fds, SizeIs(2));
    EXPECT_THAT(fds, Contains(11));
    EXPECT_THAT(fds, Contains(12));
}

namespace
{
    class TestException: public std::exception
    {
    public:
        TestException() = default;

        ~TestException() noexcept = default;

        const char *what() const noexcept { return "test exception"; }
    };
}

TEST(ListeningSocket, exceptionsFromAcceptArePassedToHandler)
{
    InSequence dummy;
    DummyFDListenerMap flm;
    SystemMock sm;
    std::string err;
    ListeningSocketMock ls(sm,
                           flm,
                           SOCK_STREAM,
                           0,
                           SocketAddress::createUnix("path"),
                           [] (FileDescriptor &, const SocketAddress &) { },
                           [&err] (const std::exception &e) { err += e.what(); });
    EXPECT_CALL(sm, accept(_, _, _))
        .Times(1)
        .WillOnce(Throw(TestException()));
    ls.handleEvents(1, FDListener::EVENT_IN);
    EXPECT_EQ(TestException().what(), err);
}

TEST(ListeningSocket, ifHandlerIsNotSetExceptionsAreNotCaught)
{
    InSequence dummy;
    DummyFDListenerMap flm;
    SystemMock sm;
    ListeningSocketMock ls(sm,
                           flm,
                           SOCK_STREAM,
                           0,
                           SocketAddress::createUnix("path"),
                           [] (FileDescriptor &, const SocketAddress &) { });
    EXPECT_CALL(sm, accept(_, _, _))
        .Times(1)
        .WillOnce(Throw(TestException()));
    EXPECT_THROW(ls.handleEvents(1, FDListener::EVENT_IN), TestException);
}

TEST(ListeningSocket, ifNewAcceptedSocketCbThrowsListeningSocketClosesTheAcceptedFD)
{
    InSequence dummy;
    DummyFDListenerMap flm;
    SystemMock sm;
    EXPECT_CALL(sm, socket(_, _, _))
        .Times(1)
        .WillOnce(Return(555));
    ListeningSocketMock ls(sm,
                           flm,
                           SOCK_STREAM,
                           0,
                           SocketAddress::createUnix("path"),
                           [] (FileDescriptor &, const SocketAddress &) { throw TestException(); },
                           [] (const std::exception &) { });
    EXPECT_CALL(sm, accept(_, _, _))
        .Times(1)
        .WillOnce(Return(11));
    EXPECT_CALL(sm, close(11))
        .Times(1);
    ls.handleEvents(1, FDListener::EVENT_IN);
    EXPECT_CALL(sm, close(555))
        .Times(1);
}

TEST(ListeningSocket, dump)
{
    DummyFDListenerMap flm;
    SystemMock sm;
    ListeningSocketMock ls(sm, flm, SOCK_STREAM, IPPROTO_TCP, SocketAddress::createIPv4("10.10.10.10", "10"), ListeningSocket::NewAcceptedSocketCb());
    EXPECT_VALID_DUMP(ls);
}
