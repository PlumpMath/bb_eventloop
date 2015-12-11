#include <gtest/gtest.h>

#include <EventLoop/Timer.hpp>

#include "private/TimerQueue.hpp"
#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/TimerQueueMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    class TimerQueueTest: public testing::Test
    {
    public:
        SystemMock sm;
        TimerQueueMock tqm;

        TimerQueueTest(): tqm(sm)
        {
            ON_CALL(sm, getMonotonicClock())
                .WillByDefault(Return(boost::posix_time::milliseconds(123)));
        }
    };
}

TEST_F(TimerQueueTest, popTimerFromEmptyQueueReturnsNull)
{
    EXPECT_EQ(nullptr, tqm.tq.popFirstTimerToExecute());
}

TEST_F(TimerQueueTest, getTimeoutFromEmptyQueueReturnsNoTimeout)
{
    EXPECT_EQ(TimerQueue::NO_TIMEOUT, tqm.tq.getTimeout());
}

TEST_F(TimerQueueTest, timersAddedWithTheExactSameTimeToExecuteAreReturnedInAdditionOrder)
{
    Timer t1(tqm);
    Timer t2(tqm);
    const boost::posix_time::time_duration timeout;
    t1.schedule(timeout, [] () { });
    t2.schedule(timeout, [] () { });
    EXPECT_EQ(&t1, tqm.tq.popFirstTimerToExecute());
    EXPECT_EQ(&t2, tqm.tq.popFirstTimerToExecute());
}

TEST_F(TimerQueueTest, dump)
{
    Timer t(tqm);
    t.schedule(boost::posix_time::seconds(10), [] () { });
    EXPECT_VALID_DUMP(tqm.tq);
}
