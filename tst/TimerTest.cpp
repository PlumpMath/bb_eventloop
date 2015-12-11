#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/Timer.hpp>

#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/TimerQueueMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    class TimerTest: public testing::Test
    {
    public:
        SystemMock sm;
        TimerQueueMock tqm;
        bool executed;

        TimerTest(): tqm(sm), executed(false)
        {
            ON_CALL(sm, getMonotonicClock())
                .WillByDefault(Return(boost::posix_time::milliseconds(0)));
        }

        void execute() { executed = true; }
    };

    class TestException: public std::exception
    {
    };
}

TEST_F(TimerTest, constructorDoesntSchedule)
{
    Timer t(tqm);
    EXPECT_FALSE(t.isScheduled());
}

TEST_F(TimerTest, scheduleAddsIntoQueue)
{
    Timer t(tqm);
    EXPECT_FALSE(t.isScheduled());
    EXPECT_TRUE(tqm.tq.isEmpty());
    t.schedule(boost::posix_time::seconds(10), [] () { });
    EXPECT_TRUE(t.isScheduled());
    EXPECT_FALSE(tqm.tq.isEmpty());
}

TEST_F(TimerTest, destructorCancels)
{
    {
        Timer t(tqm);
        t.schedule(boost::posix_time::seconds(10), [] () { });
    }
    EXPECT_TRUE(tqm.tq.isEmpty());
}

TEST_F(TimerTest, scheduleKeepsTimersInCorrectOrder)
{
    Timer t1(tqm);
    Timer t2(tqm);
    Timer t3(tqm);

    t2.schedule(boost::posix_time::seconds(20), [] () { });
    t1.schedule(boost::posix_time::seconds(10), [] () { });
    t3.schedule(boost::posix_time::seconds(30), [] () { });

    EXPECT_EQ(&t1, tqm.tq.popFirstTimerToExecute());
    EXPECT_EQ(&t2, tqm.tq.popFirstTimerToExecute());
    EXPECT_EQ(&t3, tqm.tq.popFirstTimerToExecute());
    EXPECT_EQ(nullptr, tqm.tq.popFirstTimerToExecute());
}

TEST_F(TimerTest, scheduleRemovesOldInstance)
{
    Timer t(tqm);
    t.schedule(boost::posix_time::seconds(20), [] () { });
    t.schedule(boost::posix_time::seconds(10), [] () { });
    EXPECT_EQ(boost::posix_time::seconds(10), tqm.tq.getTimeout());
    EXPECT_EQ(&t, tqm.tq.popFirstTimerToExecute());
    EXPECT_EQ(nullptr, tqm.tq.popFirstTimerToExecute());
}

TEST_F(TimerTest, scheduleNullCallbackThrows)
{
    Timer t(tqm);
    EXPECT_THROW(t.schedule(boost::posix_time::seconds(20), std::function<void()>()), Timer::NullCallback);
}

TEST_F(TimerTest, cancelRemovesOldInstance)
{
    Timer t(tqm);
    t.schedule(boost::posix_time::seconds(10), [] () { });
    t.cancel();
    EXPECT_FALSE(t.isScheduled());
    EXPECT_TRUE(tqm.tq.isEmpty());
}

TEST_F(TimerTest, getTimeoutAfterSchedulingReturnsRealTimeout)
{
    Timer t(tqm);
    t.schedule(boost::posix_time::seconds(20), [] () { });
    EXPECT_EQ(boost::posix_time::seconds(20), tqm.tq.getTimeout());
}

TEST_F(TimerTest, getTimeoutWhenAlreadyLateReturnsZero)
{
    Timer t(tqm);

    t.schedule(boost::posix_time::seconds(10), [] () { });

    /* Time flies 100 seconds... */
    ON_CALL(sm, getMonotonicClock())
        .WillByDefault(Return(boost::posix_time::milliseconds(100 * 1000)));

    EXPECT_EQ(boost::posix_time::seconds(0), tqm.tq.getTimeout());
}

TEST_F(TimerTest, scheduleAndExecute)
{
    Timer t(tqm);
    t.schedule(boost::posix_time::seconds(1), std::bind(&TimerTest::execute, this));
    t.execute();
    EXPECT_TRUE(executed);
}

TEST_F(TimerTest, cancelClearsCallback)
{
    std::shared_ptr<int> sp(std::make_shared<int>(123));
    std::weak_ptr<int> wp(sp);
    Timer t(tqm);
    t.schedule(boost::posix_time::seconds(1), [sp] () { static_cast<void>(sp); });
    sp.reset();
    t.cancel();
    EXPECT_EQ(nullptr, wp.lock());
}

TEST_F(TimerTest, executionClearsCallback)
{
    Timer t(tqm);
    t.schedule(boost::posix_time::seconds(1), [] () { });
    t.execute();
    EXPECT_THROW(t.execute(), std::bad_function_call);
}

TEST_F(TimerTest, executionClearsCallbackEvenIfExecuteThrows)
{
    Timer t(tqm);
    t.schedule(boost::posix_time::seconds(1), [] () { throw TestException(); });
    EXPECT_THROW(t.execute(), TestException);
    EXPECT_THROW(t.execute(), std::bad_function_call);
}

TEST_F(TimerTest, executionCanScheduleTheTimerAgain)
{
    bool executed(false);
    Timer t(tqm);
    t.schedule(boost::posix_time::seconds(1),
               [&t, &executed] ()
               {
                   t.schedule(boost::posix_time::seconds(1),
                              [&executed] ()
                              {
                                  executed = true;
                              });
               });
    t.execute();
    t.execute();
    EXPECT_TRUE(executed);
}

TEST_F(TimerTest, dump)
{
    Timer t(tqm);
    EXPECT_VALID_DUMP(t);
}
