#include <memory>
#include <sys/epoll.h>
#include <gtest/gtest.h>

#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/FDListenerMock.hpp"
#include "private/tst/BaseEpollRunnerMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    class BaseEpollRunnerTest: public testing::Test
    {
    public:
        SystemMock sm;
        FDListenerMock fl;
        std::unique_ptr<BaseEpollRunnerMock> ber;

        BaseEpollRunnerTest()
        {
            EXPECT_CALL(sm, epollCtl(_, _, _, _))
                .Times(AnyNumber());
            ber.reset(new BaseEpollRunnerMock(sm));
        }
    };
}

TEST_F(BaseEpollRunnerTest, addFDListenerUpdatesEpoll)
{
    EXPECT_CALL(sm, epollCtl(_, EPOLL_CTL_ADD, 123, _))
        .Times(1);
    ber->fdListenerMap.add(fl, 123, FDListener::EVENT_IN);
}

TEST_F(BaseEpollRunnerTest, modifyFDListenerUpdatesEpoll)
{
    ber->fdListenerMap.add(fl, 123, FDListener::EVENT_IN);
    EXPECT_CALL(sm, epollCtl(_, EPOLL_CTL_MOD, 123, _))
        .Times(1);
    ber->fdListenerMap.modify(123, FDListener::EVENT_OUT);
}

TEST_F(BaseEpollRunnerTest, delFDListenerUpdatesEpoll)
{
    ber->fdListenerMap.add(fl, 123, FDListener::EVENT_IN);
    EXPECT_CALL(sm, epollCtl(_, EPOLL_CTL_DEL, 123, _))
        .Times(1);
    ber->fdListenerMap.del(123);
}

TEST_F(BaseEpollRunnerTest, dump)
{
    EXPECT_VALID_DUMP(*ber);
}
