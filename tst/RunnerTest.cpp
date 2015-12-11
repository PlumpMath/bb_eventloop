#include <gtest/gtest.h>

#include <EventLoop/Runner.hpp>

using namespace testing;
using namespace EventLoop;

TEST(Runner, fdAlreadyAdded)
{
    const Runner::FDAlreadyAdded e(123);
    const std::exception &se(e);
    EXPECT_STREQ("fd 123 already added", se.what());
}

TEST(Runner, nonExistingFDListener)
{
    const Runner::NonExistingFDListener e(123);
    const std::exception &se(e);
    EXPECT_STREQ("non-existing FDListener: 123", se.what());
}
