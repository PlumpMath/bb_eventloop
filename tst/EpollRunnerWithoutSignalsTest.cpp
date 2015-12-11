#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <pthread.h>
#include <gtest/gtest.h>

#include <EventLoop/Timer.hpp>
#include <EventLoop/SignalSet.hpp>

#include "private/TimerQueue.hpp"
#include "private/CallbackQueue.hpp"
#include "private/FDListenerMap.hpp"
#include "private/System.hpp"
#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/FDListenerMock.hpp"
#include "private/tst/EpollRunnerMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    void f() { }

    bool dontContinue() { return false; }

    class EpollRunnerWithoutSignals: public testing::Test
    {
    public:
        SystemMock sm;
        std::unique_ptr<EpollRunnerMock> er;
        std::string str;

        void createER()
        {
            int fd[2] = { 14, 15 };

            ON_CALL(sm, getMonotonicClock())
                .WillByDefault(Return(boost::posix_time::milliseconds(0)));
            EXPECT_CALL(sm, pipe(_, O_CLOEXEC))
                .Times(1)
                .WillOnce(SetArrayArgument<0>(fd, fd + 2));
            er.reset(new EpollRunnerMock(sm));
        }

        void destroyER()
        {
            er.reset();
        }

        void execute(const std::string &s) { str += s; }
    };

    class CreatedEpollRunnerWithoutSignals: public EpollRunnerWithoutSignals
    {
    public:
        CreatedEpollRunnerWithoutSignals() { createER(); }

        ~CreatedEpollRunnerWithoutSignals() { destroyER(); }
    };
}

TEST_F(EpollRunnerWithoutSignals, addedFDListenersAreHandled)
{
    struct epoll_event expectedEvents[2] = { };
    expectedEvents[0].events = EPOLLIN;
    expectedEvents[0].data.fd = 111;
    expectedEvents[1].events = EPOLLOUT;
    expectedEvents[1].data.fd = 333;

    createER();
    FDListenerMock fl1;
    FDListenerMock fl2;
    FDListenerMock fl3;
    er->addFDListener(fl1, 111, FDListener::EVENT_IN);
    er->addFDListener(fl2, 222, FDListener::EVENT_OUT);
    er->addFDListener(fl3, 333, FDListener::EVENT_OUT);
    EXPECT_CALL(sm, epollWait(_, _, Ge(3), -1))
        .Times(1)
        .WillOnce(DoAll(SetArrayArgument<1>(expectedEvents, expectedEvents + 2),
                        Return(2)));
    EXPECT_CALL(fl1, handleEventsCb(111, FDListener::EVENT_IN))
        .Times(1);
    EXPECT_CALL(fl2, handleEventsCb(222, FDListener::EVENT_OUT))
        .Times(0);
    EXPECT_CALL(fl3, handleEventsCb(333, FDListener::EVENT_OUT))
        .Times(1);
    er->run(dontContinue);
    destroyER();
}

TEST_F(EpollRunnerWithoutSignals, deletingMultipleFDsWhileHandlingEventsOfOneFDIsPossibleEvenIfThereArePendingEventsForTheOtherFDs)
{
    const int fd1(111);
    const int fd2(222);
    struct epoll_event expectedEvents[2] = { };
    expectedEvents[0].events = EPOLLOUT;
    expectedEvents[0].data.fd = fd1;
    expectedEvents[1].events = EPOLLOUT;
    expectedEvents[1].data.fd = fd2;

    createER();
    FDListenerMock fl1;
    FDListenerMock fl2;
    er->addFDListener(fl1, fd1, FDListener::EVENT_OUT);
    er->addFDListener(fl2, fd2, FDListener::EVENT_OUT);
    EXPECT_CALL(sm, epollWait(_, _, Ge(2), -1))
        .Times(1)
        .WillOnce(DoAll(SetArrayArgument<1>(expectedEvents, expectedEvents + 2),
                        Return(2)));
    EXPECT_CALL(fl1, handleEventsCb(fd1, FDListener::EVENT_OUT))
        .Times(1)
        .WillOnce(Invoke([this, fd2] (int, unsigned int)
                         {
                             er->delFDListener(fd2);
                         }));
    EXPECT_CALL(fl2, handleEventsCb(fd2, _))
        .Times(0);
    er->run(dontContinue);
    destroyER();
}

TEST_F(CreatedEpollRunnerWithoutSignals, noTimersNoTimeout)
{
    EXPECT_CALL(sm, epollWait(_, _, _, -1))
        .Times(1);
    er->run(dontContinue);
}

TEST_F(CreatedEpollRunnerWithoutSignals, timeoutIsTakenFromTheFirstTimer)
{
    Timer t(*er);
    t.schedule(boost::posix_time::seconds(10), f);
    EXPECT_CALL(sm, epollWait(_, _, _, 10000))
        .Times(1);
    er->run(dontContinue);
}

TEST_F(CreatedEpollRunnerWithoutSignals, afterTimeoutTheFirstTimerIsPoppedAndExecuted)
{
    Timer t1(*er);
    Timer t2(*er);
    t1.schedule(boost::posix_time::seconds(10), std::bind(&EpollRunnerWithoutSignals::execute, this, "1"));
    t2.schedule(boost::posix_time::seconds(20), std::bind(&EpollRunnerWithoutSignals::execute, this, "2"));
    EXPECT_CALL(sm, epollWait(_, _, _, _))
        .Times(1)
        .WillOnce(Return(0));
    er->run(dontContinue);
    EXPECT_EQ("1", str);
    EXPECT_FALSE(t1.isScheduled());
    EXPECT_TRUE(t2.isScheduled());
}

TEST_F(CreatedEpollRunnerWithoutSignals, zeroTimeoutTimersAreExecutedBeforeEpollWait)
{
    Timer t1(*er);
    Timer t2(*er);
    t1.schedule(boost::posix_time::seconds(0), std::bind(&EpollRunnerWithoutSignals::execute, this, "1"));
    t2.schedule(boost::posix_time::seconds(0), std::bind(&EpollRunnerWithoutSignals::execute, this, "2"));
    EXPECT_CALL(sm, epollWait(_, _, _, _))
        .Times(0);
    er->run(dontContinue);
    EXPECT_EQ("1", str);
    EXPECT_FALSE(t1.isScheduled());
    EXPECT_TRUE(t2.isScheduled());
}

TEST_F(CreatedEpollRunnerWithoutSignals, addedCallbacksAreExecuted)
{
    er->addCallback(std::bind(&EpollRunnerWithoutSignals::execute, this, "1"));
    er->addCallback(std::bind(&EpollRunnerWithoutSignals::execute, this, "2"));
    er->run(dontContinue);
    EXPECT_EQ("12", str);
}

TEST_F(CreatedEpollRunnerWithoutSignals, settingSignalsThrowNoSignalHandling)
{
    EXPECT_THROW(er->setSignals(SignalSet({SIGHUP})), EpollRunner::NoSignalHandling);
}
