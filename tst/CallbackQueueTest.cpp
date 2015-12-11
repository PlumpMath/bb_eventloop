#include <string>
#include <exception>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/Runner.hpp>

#include "private/CallbackQueue.hpp"
#include "private/tst/DumpTest.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    class TestException: public std::exception { };

    class CallbackQueueTest: public testing::Test
    {
    public:
        CallbackQueue cbq;
        std::string str;
        int wakeUpCounter;

        CallbackQueueTest(): wakeUpCounter(0) { }

        void execute(const std::string &s) { str += s; }

        void executeAndAdd(const std::string &s)
        {
            cbq.add(std::bind(&CallbackQueueTest::execute, this, s));
        }

        void wakeUp() { ++wakeUpCounter; }
    };

    void f() { }

    void throwTestException() { throw TestException(); }
}

TEST_F(CallbackQueueTest, addedCallbacksAreExecutedInOrder)
{
    cbq.add(std::bind(&CallbackQueueTest::execute, this, "1"));
    cbq.add(std::bind(&CallbackQueueTest::execute, this, "2"));
    cbq.execute();
    EXPECT_EQ("12", str);
}

TEST_F(CallbackQueueTest, callbackCanAddNewCallbacksWhichAreAlsoExecuted)
{
    cbq.add(std::bind(&CallbackQueueTest::executeAndAdd, this, "yes"));
    cbq.execute();
    EXPECT_EQ("yes", str);
}

TEST_F(CallbackQueueTest, exceptionStopsExecutionAndNextExecuteCanContinueNormally)
{
    cbq.add(std::bind(&CallbackQueueTest::execute, this, "1"));
    cbq.add(throwTestException);
    cbq.add(std::bind(&CallbackQueueTest::execute, this, "2"));
    EXPECT_THROW(cbq.execute(), TestException);
    EXPECT_EQ("1", str);
    cbq.execute();
    EXPECT_EQ("12", str);
}

TEST_F(CallbackQueueTest, wakeUpIsCalledWhenFirstCallbackIsAdded)
{
    cbq.setWakeUp(std::bind(&CallbackQueueTest::wakeUp, this));
    cbq.add(f);
    EXPECT_EQ(1, wakeUpCounter);
    cbq.add(f);
    EXPECT_EQ(1, wakeUpCounter);
}

TEST_F(CallbackQueueTest, addingNullCallbackThrows)
{
    EXPECT_THROW(cbq.add(std::function<void()>()), Runner::NullCallback);
}

TEST_F(CallbackQueueTest, executeEmptyQueueReturnsFalse)
{
    EXPECT_FALSE(cbq.execute());
}

TEST_F(CallbackQueueTest, executeNonEmptyQueueReturnsTrue)
{
    cbq.add(f);
    EXPECT_TRUE(cbq.execute());
}

TEST_F(CallbackQueueTest, dump)
{
    cbq.add(f);
    EXPECT_VALID_DUMP(cbq);
}
