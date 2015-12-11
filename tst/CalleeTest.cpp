#include <gtest/gtest.h>

#include <EventLoop/Callee.hpp>

#include "private/tst/DumpTest.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

TEST(Callee, dump)
{
    Callee c;
    EXPECT_VALID_DUMP(c);
}
