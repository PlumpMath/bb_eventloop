#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "private/Epoll.hpp"
#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

TEST(Epoll, constructorCreatesAndDestructorClosesFD)
{
    SystemMock sm;
    EXPECT_CALL(sm, epollCreate(EPOLL_CLOEXEC))
        .Times(1)
        .WillOnce(Return(123));
    EXPECT_CALL(sm, close(123))
        .Times(1);
    Epoll ep(sm);
    EXPECT_EQ(123, ep.getFD());
}

TEST(Epoll, add)
{
    SystemMock sm;
    struct epoll_event event;
    EXPECT_CALL(sm, epollCreate(EPOLL_CLOEXEC))
        .Times(1)
        .WillOnce(Return(123));
    Epoll ep(sm);
    EXPECT_CALL(sm, epollCtl(123, EPOLL_CTL_ADD, 222, &event))
        .Times(1);
    ep.add(222, &event);
}

TEST(Epoll, modify)
{
    SystemMock sm;
    struct epoll_event event;
    EXPECT_CALL(sm, epollCreate(EPOLL_CLOEXEC))
        .Times(1)
        .WillOnce(Return(123));
    Epoll ep(sm);
    EXPECT_CALL(sm, epollCtl(123, EPOLL_CTL_MOD, 222, &event))
        .Times(1);
    ep.modify(222, &event);
}

TEST(Epoll, del)
{
    SystemMock sm;
    EXPECT_CALL(sm, epollCreate(EPOLL_CLOEXEC))
        .Times(1)
        .WillOnce(Return(123));
    Epoll ep(sm);
    EXPECT_CALL(sm, epollCtl(123, EPOLL_CTL_DEL, 222, _))
        .Times(1);
    ep.del(222);
}

TEST(Epoll, waitWithoutFDs)
{
    SystemMock sm;
    EXPECT_CALL(sm, epollCreate(EPOLL_CLOEXEC))
        .Times(1)
        .WillOnce(Return(333));
    Epoll ep(sm);
    EXPECT_CALL(sm, epollWait(333, IsNull(), 0, -1))
        .Times(1);
    ep.wait(-1);
}

TEST(Epoll, waitWithTimeout)
{
    SystemMock sm;
    EXPECT_CALL(sm, epollCreate(EPOLL_CLOEXEC))
        .Times(1)
        .WillOnce(Return(2));
    Epoll ep(sm);
    struct epoll_event event;
    ep.add(3, &event);
    ep.add(4, &event);
    ep.add(5, &event);
    EXPECT_CALL(sm, epollWait(2, _, Ge(3), 1234))
        .Times(1);
    ep.wait(1234);
}

TEST(Epoll, timeoutWaitReturnsTrue)
{
    SystemMock sm;
    EXPECT_CALL(sm, epollCreate(EPOLL_CLOEXEC))
        .Times(1)
        .WillOnce(Return(123));
    Epoll ep(sm);
    struct epoll_event event;
    ep.add(222, &event);
    EXPECT_CALL(sm, epollWait(123, _, Ge(1), 1))
        .Times(1)
        .WillOnce(Return(0));
    bool timeOut(ep.wait(1));
    EXPECT_TRUE(timeOut);
}

TEST(Epoll, successfulWaitReturnsFalse)
{
    SystemMock sm;
    EXPECT_CALL(sm, epollCreate(EPOLL_CLOEXEC))
        .Times(1)
        .WillOnce(Return(123));
    Epoll ep(sm);
    struct epoll_event event;
    ep.add(222, &event);
    EXPECT_CALL(sm, epollWait(123, _, Ge(1), 1))
        .Times(1)
        .WillOnce(Return(1));
    bool timeOut(ep.wait(1));
    EXPECT_FALSE(timeOut);
}

TEST(Epoll, eventIteratorWithoutResults)
{
    SystemMock sm;
    Epoll ep(sm);

    EXPECT_EQ(ep.events_begin(), ep.events_end());
}

TEST(Epoll, eventIteratorAfterSuccessfulWait)
{
    SystemMock sm;
    Epoll ep(sm);
    struct epoll_event event;
    ep.add(1, &event);
    ep.add(2, &event);
    ep.add(3, &event);
    ep.add(4, &event);
    EXPECT_CALL(sm, epollWait(_, _, _, _))
        .Times(1)
        .WillOnce(Return(4));
    ep.wait(-1);
    int count(0);
    for (Epoll::event_iterator i = ep.events_begin();
         i != ep.events_end();
         ++i)
    {
        ++count;
    }
    EXPECT_EQ(4, count);
}

TEST(Epoll, accessIterator)
{
    struct epoll_event expectedEvents[2] = { { EPOLLIN }, { EPOLLHUP } };

    SystemMock sm;
    Epoll ep(sm);
    struct epoll_event event;
    ep.add(1, &event);
    ep.add(2, &event);
    EXPECT_CALL(sm, epollWait(_, _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArrayArgument<1>(expectedEvents, expectedEvents + 2),
                        Return(2)));
    ep.wait(-1);

    Epoll::event_iterator i(ep.events_begin());
    ASSERT_NE(ep.events_end(), i);
    EXPECT_EQ(static_cast<unsigned int>(EPOLLIN), i->events);
    ++i;
    ASSERT_NE(ep.events_end(), i);
    EXPECT_EQ(static_cast<unsigned int>(EPOLLHUP), i->events);
}

TEST(Epoll, dump)
{
    SystemMock sm;
    Epoll ep(sm);
    EXPECT_VALID_DUMP(ep);
}
