#include <gtest/gtest.h>

#include "private/System.hpp"

TEST(System, canCreateInstance)
{
    EventLoop::System s;
    static_cast<void>(s);
}

TEST(System, getSystem)
{
    EventLoop::System &s(EventLoop::System::getSystem());
    static_cast<void>(s);
}
