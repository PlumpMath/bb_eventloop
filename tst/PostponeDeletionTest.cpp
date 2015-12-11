#include <gtest/gtest.h>

#include <EventLoop/PostponeDeletion.hpp>
#include <EventLoop/EpollRunner.hpp>

using namespace testing;
using namespace EventLoop;

TEST(PostponeDeletion, postponeDeletionKeepsReferenceUntilTheNextRunLoop)
{
    std::shared_ptr<int> sp(std::make_shared<int>(123));
    std::weak_ptr<int> wp(sp);
    EpollRunner runner;
    postponeDeletion(runner, sp);
    sp.reset();
    EXPECT_FALSE(wp.expired());
    runner.run([] () { return false; });
    EXPECT_TRUE(wp.expired());
}
