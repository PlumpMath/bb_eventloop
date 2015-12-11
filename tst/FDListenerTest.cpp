#include <gtest/gtest.h>

#include <EventLoop/FDListener.hpp>

#include "private/tst/DumpTest.hpp"
#include "private/tst/FDListenerMock.hpp"

using namespace EventLoop;
using namespace EventLoop::Test;

TEST(FDListener, setAndGetEvents)
{
    FDListenerMock fl;
    EXPECT_CALL(fl, handleEventsCb(1, FDListener::EVENT_IN | FDListener::EVENT_OUT))
        .Times(1);
    fl.handleEvents(1, FDListener::EVENT_IN | FDListener::EVENT_OUT);
}

TEST(FDListener, dump)
{
    FDListenerMock fl;
    EXPECT_VALID_DUMP(fl);
}
