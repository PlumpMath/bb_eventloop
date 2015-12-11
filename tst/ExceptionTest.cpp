#include <gtest/gtest.h>

#include <EventLoop/Exception.hpp>

using namespace EventLoop;

TEST(Exception, stringConstructor)
{
    Exception e("test");
    const std::exception &s(e);
    EXPECT_EQ(std::string("test"), std::string(s.what()));
}
