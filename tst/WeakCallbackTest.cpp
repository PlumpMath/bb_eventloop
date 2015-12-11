#include <gtest/gtest.h>

#include <EventLoop/WeakCallback.hpp>

#include "private/CallbackQueue.hpp"

using namespace testing;
using namespace EventLoop;

namespace
{
    class WeakCallbackTest: public testing::Test
    {
    public:
        int counter;
        std::unique_ptr<Callee> callee;

        WeakCallbackTest(): counter(0), callee(new Callee()) { }

        void doit() { ++counter; }

        void EXPECT_DONE() { EXPECT_EQ(1, counter); }

        void EXPECT_NOT_DONE() { EXPECT_EQ(0, counter); }
    };
}

TEST_F(WeakCallbackTest, callbackCanBeGivenInConstructorAndCalled)
{
    WeakCallback wcb(*callee, std::bind(&WeakCallbackTest::doit, this));
    wcb();
    EXPECT_DONE();
}

TEST_F(WeakCallbackTest, callbackDoesNothingIfCalleeIsDestroyed)
{
    WeakCallback wcb(*callee, std::bind(&WeakCallbackTest::doit, this));
    callee.reset();
    wcb();
    EXPECT_NOT_DONE();
}

TEST_F(WeakCallbackTest, waekCallbackIsCompatibleWithCallbackQueue)
{
    CallbackQueue q;
    WeakCallback wcb(*callee, std::bind(&WeakCallbackTest::doit, this));
    q.add(wcb);
    q.execute();
    EXPECT_DONE();
}

TEST_F(WeakCallbackTest, waekCallbackIsCompatibleWithCallbackQueueAlsoWhenCalleeIsDestroyed)
{
    CallbackQueue q;
    q.add(WeakCallback(*callee, std::bind(&WeakCallbackTest::doit, this)));
    callee.reset();
    q.execute();
    EXPECT_NOT_DONE();
}
