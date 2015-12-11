#include <string>
#include <sys/epoll.h>
#include <gtest/gtest.h>

#include <EventLoop/Timer.hpp>

#include "private/TimerQueue.hpp"
#include "private/CallbackQueue.hpp"
#include "private/FDListenerMap.hpp"
#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/FDListenerMock.hpp"
#include "private/tst/AsyncRunnerMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    void f() { }
}

TEST(AsyncRunner, constructorCreatesTimerFDDestructorDeletesIt)
{
    SystemMock sm;
    EXPECT_CALL(sm, timerFDCreate(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
        .Times(1)
        .WillOnce(Return(222));
    AsyncRunnerMock ar(sm);
    EXPECT_CALL(sm, close(_))
        .Times(AnyNumber());
    EXPECT_CALL(sm, close(222))
        .Times(1);
}

TEST(AsyncRunner, getEventFDReturnsTheEpollFD)
{
    SystemMock sm;
    EXPECT_CALL(sm, epollCreate(_))
        .Times(1)
        .WillOnce(Return(124));
    AsyncRunnerMock ar(sm);
    EXPECT_EQ(124, ar.getEventFD());
}

TEST(AsyncRunner, epollWaitIsCalledWithZeroTimeoutInStep)
{
    SystemMock sm;
    AsyncRunnerMock ar(sm);
    EXPECT_CALL(sm, epollWait(_, _, _, 0))
        .Times(1)
        .WillOnce(Return(0));
    ar.step();
}

TEST(AsyncRunner, addedFDListenersAreHandled)
{
    struct epoll_event expectedEvents[2] = { };
    expectedEvents[0].events = EPOLLIN;
    expectedEvents[0].data.fd = 111;
    expectedEvents[1].events = EPOLLOUT;
    expectedEvents[1].data.fd = 333;

    SystemMock sm;
    AsyncRunnerMock ar(sm);
    FDListenerMock fl1;
    FDListenerMock fl2;
    FDListenerMock fl3;
    ar.addFDListener(fl1, 111, FDListener::EVENT_IN);
    ar.addFDListener(fl2, 222, FDListener::EVENT_OUT);
    ar.addFDListener(fl3, 333, FDListener::EVENT_OUT);
    EXPECT_CALL(sm, epollWait(_, _, Ge(3), 0))
        .Times(1)
        .WillOnce(DoAll(SetArrayArgument<1>(expectedEvents, expectedEvents + 2),
                        Return(2)));
    EXPECT_CALL(fl1, handleEventsCb(111, FDListener::EVENT_IN))
        .Times(1);
    EXPECT_CALL(fl2, handleEventsCb(222, FDListener::EVENT_OUT))
        .Times(0);
    EXPECT_CALL(fl3, handleEventsCb(333, FDListener::EVENT_OUT))
        .Times(1);
    ar.step();
}

TEST(AsyncRunner, deletingMultipleFDsWhileHandlingEventsOfOneFDIsPossibleEvenIfThereArePendingEventsForTheOtherFDs)
{
    const int fd1(111);
    const int fd2(222);
    struct epoll_event expectedEvents[2] = { };
    expectedEvents[0].events = EPOLLOUT;
    expectedEvents[0].data.fd = fd1;
    expectedEvents[1].events = EPOLLOUT;
    expectedEvents[1].data.fd = fd2;

    SystemMock sm;
    AsyncRunnerMock ar(sm);
    FDListenerMock fl1;
    FDListenerMock fl2;
    ar.addFDListener(fl1, fd1, FDListener::EVENT_OUT);
    ar.addFDListener(fl2, fd2, FDListener::EVENT_OUT);
    EXPECT_CALL(sm, epollWait(_, _, Ge(2), 0))
        .Times(1)
        .WillOnce(DoAll(SetArrayArgument<1>(expectedEvents, expectedEvents + 2),
                        Return(2)));
    EXPECT_CALL(fl1, handleEventsCb(fd1, FDListener::EVENT_OUT))
        .Times(1)
        .WillOnce(Invoke([&ar, fd2] (int, unsigned int)
                         {
                             ar.delFDListener(fd2);
                         }));
    EXPECT_CALL(fl2, handleEventsCb(fd2, _))
        .Times(0);
    ar.step();
}

bool operator == (const struct itimerspec &a, const struct itimerspec &b)
{
    return ((a.it_interval.tv_sec == b.it_interval.tv_sec) &&
            (a.it_interval.tv_nsec == b.it_interval.tv_nsec) &&
            (a.it_value.tv_sec == b.it_value.tv_sec) &&
            (a.it_value.tv_nsec == b.it_value.tv_nsec));
}

TEST(AsyncRunner, callingStepWhenThereAreNoTimersDisarmsTimer)
{
    struct itimerspec expectedSpec = { { 0, 0 }, { 0, 0 } };

    SystemMock sm;
    AsyncRunnerMock ar(sm);

    EXPECT_CALL(sm, timerFDSetTime(_, 0, Pointee(expectedSpec), IsNull()))
        .Times(1);

    ar.step();
}

TEST(AsyncRunner, callingStepWhenThereAreTimersArmsTimer)
{
    struct itimerspec expectedSpec = { { 0, 0 }, { 200, 40000000 } };

    SystemMock sm;

    ON_CALL(sm, getMonotonicClock())
        .WillByDefault(Return(boost::posix_time::milliseconds(0)));

    AsyncRunnerMock ar(sm);
    Timer t(ar);
    t.schedule(boost::posix_time::milliseconds(200040), f);

    EXPECT_CALL(sm, timerFDSetTime(_, 0, Pointee(expectedSpec), IsNull()))
        .Times(1);

    ar.step();
}

namespace
{
    class AsyncRunnerTest: public testing::Test
    {
    public:
        AsyncRunnerTest(): ar(sm) { }

        SystemMock sm;
        AsyncRunnerMock ar;
        std::string str;

        void execute(const std::string &s) { str += s; }
    };
}

TEST_F(AsyncRunnerTest, callingStepWhenThereAreTimersJustOnTimeOrLateExecutesTheTimersAndThenArmsTheTimer)
{
    struct itimerspec expectedSpec = { { 0, 0 }, { 104, 0 } };

    ON_CALL(sm, getMonotonicClock())
        .WillByDefault(Return(boost::posix_time::milliseconds(0)));

    Timer late(ar);
    late.schedule(boost::posix_time::seconds(99), std::bind(&AsyncRunnerTest::execute, this, "1"));
    Timer justInTime(ar);
    justInTime.schedule(boost::posix_time::seconds(100), std::bind(&AsyncRunnerTest::execute, this, "2"));
    Timer inTheFuture(ar);
    inTheFuture.schedule(boost::posix_time::seconds(204), std::bind(&AsyncRunnerTest::execute, this, "3"));

    /* Time flies 100 seconds... */
    ON_CALL(sm, getMonotonicClock())
        .WillByDefault(Return(boost::posix_time::milliseconds(100 * 1000)));

    EXPECT_CALL(sm, timerFDSetTime(_, 0, Pointee(expectedSpec), IsNull()))
        .Times(1);

    ar.step();
    EXPECT_EQ("12", str);
}

TEST_F(AsyncRunnerTest, addedCallbacksAreExecuted)
{
    ar.addCallback(std::bind(&AsyncRunnerTest::execute, this, "1"));
    ar.addCallback(std::bind(&AsyncRunnerTest::execute, this, "2"));
    ar.step();
    EXPECT_EQ("12", str);
}

TEST(AsyncRunner, eventInTimerFDIsHandled)
{
    SystemMock sm;
    EXPECT_CALL(sm, timerFDCreate(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
        .Times(1)
        .WillOnce(Return(234));
    AsyncRunnerMock ar(sm);

    struct epoll_event expectedEvents[1] = { };
    expectedEvents[0].events = EPOLLIN;
    expectedEvents[0].data.fd = 234;

    EXPECT_CALL(sm, epollWait(_, _, _, 0))
        .Times(1)
        .WillOnce(DoAll(SetArrayArgument<1>(expectedEvents, expectedEvents + 1),
                        Return(1)));

    EXPECT_CALL(sm, read(234, _, _))
        .Times(2)
        .WillOnce(Return(8))
        .WillOnce(Return(-1));

    ar.step();
}

TEST(AsyncRunner, callbacksCanBeUsedLikeCommandPattern)
{
    AsyncRunner runner;
    bool executed(false);
    runner.addCallback([&runner, &executed] ()
                       {
                           runner.addCallback([&runner, &executed] ()
                                              {
                                                  runner.addCallback([&runner, &executed] ()
                                                                     {
                                                                         executed = true;
                                                                     });
                                              });
                       });
    runner.step();
    EXPECT_TRUE(executed);
}

TEST(AsyncRunner, dump)
{
    SystemMock sm;

    ON_CALL(sm, getMonotonicClock())
        .WillByDefault(Return(boost::posix_time::milliseconds(0)));

    AsyncRunnerMock ar(sm);
    EXPECT_VALID_DUMP(ar);
}
