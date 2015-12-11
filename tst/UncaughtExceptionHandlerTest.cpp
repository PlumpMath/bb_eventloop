#include <exception>
#include <gtest/gtest.h>

#include <EventLoop/UncaughtExceptionHandler.hpp>

using namespace EventLoop;

TEST(UncaughtExceptionHandler, enableAndDisableMaintainsReferenceCount)
{
    EXPECT_EQ(0, UncaughtExceptionHandler::getReferenceCount());
    UncaughtExceptionHandler::enable();
    EXPECT_EQ(1, UncaughtExceptionHandler::getReferenceCount());
    UncaughtExceptionHandler::enable();
    EXPECT_EQ(2, UncaughtExceptionHandler::getReferenceCount());
    UncaughtExceptionHandler::disable();
    EXPECT_EQ(1, UncaughtExceptionHandler::getReferenceCount());
    UncaughtExceptionHandler::disable();
    EXPECT_EQ(0, UncaughtExceptionHandler::getReferenceCount());
}

TEST(UncaughtExceptionHandler, guardConstructorEnablesAndDestructorDisables)
{
    EXPECT_EQ(0, UncaughtExceptionHandler::getReferenceCount());
    {
        UncaughtExceptionHandler::Guard g1;
        EXPECT_EQ(1, UncaughtExceptionHandler::getReferenceCount());
        {
            UncaughtExceptionHandler::Guard g2;
            EXPECT_EQ(2, UncaughtExceptionHandler::getReferenceCount());
        }
        EXPECT_EQ(1, UncaughtExceptionHandler::getReferenceCount());
    }
    EXPECT_EQ(0, UncaughtExceptionHandler::getReferenceCount());
}
