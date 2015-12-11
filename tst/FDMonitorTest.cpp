#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "private/FDMonitor.hpp"
#include "private/tst/DumpTest.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    class FDMonitorTest: public testing::Test
    {
    public:
        const int fd;
        FDMonitor m;

        FDMonitorTest(): fd(123) { }
    };
}

TEST_F(FDMonitorTest, addedFDsAreMonitoredWhenMonitoringHasBeenStarted)
{
    m.startMonitoring();
    EXPECT_FALSE(m.has(fd));
    m.add(fd);
    EXPECT_TRUE(m.has(fd));
}

TEST_F(FDMonitorTest, addedFDsAreNotMonitoredIfMonitoringHasNotBeenStarted)
{
    m.add(fd);
    EXPECT_FALSE(m.has(fd));
}

TEST_F(FDMonitorTest, addedFDsAreClearedWhenMonitoringIsStopped)
{
    m.startMonitoring();
    m.add(fd);
    m.stopMonitoring();
    EXPECT_FALSE(m.has(fd));
}

TEST_F(FDMonitorTest, addedFDsAreNotMonitoredIfMonitoringHasNotBeenStopped)
{
    m.startMonitoring();
    m.stopMonitoring();
    m.add(fd);
    EXPECT_FALSE(m.has(fd));
}

TEST_F(FDMonitorTest, guard)
{
    {
        FDMonitor::Guard g(m);
        m.add(fd);
        EXPECT_TRUE(m.has(fd));
    }
    EXPECT_FALSE(m.has(fd));
}

TEST_F(FDMonitorTest, dump)
{
    EXPECT_VALID_DUMP(m);
}
