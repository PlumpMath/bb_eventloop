#include <gtest/gtest.h>

#include <EventLoop/TypeName.hpp>

#include "private/System.hpp"

using namespace testing;
using namespace EventLoop;

TEST(TypeName, nativeType)
{
    char buffer[EVENTLOOP_MAX_TYPE_NAME];
    const int i(123);
    EXPECT_STREQ("int", getTypeName(i, buffer, sizeof(buffer)));
}

TEST(TypeName, ownType)
{
    char buffer[EVENTLOOP_MAX_TYPE_NAME];
    EXPECT_STREQ("EventLoop::System", getTypeName(System::getSystem(), buffer, sizeof(buffer)));
}
