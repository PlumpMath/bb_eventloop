#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <gtest/gtest.h>

#include <EventLoop/Timer.hpp>

#include "private/TimerQueue.hpp"
#include "private/CallbackQueue.hpp"
#include "private/FDListenerMap.hpp"
#include "private/System.hpp"
#include "private/tst/DumpTest.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/FDListenerMock.hpp"
#include "private/tst/EpollRunnerMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    void f() { }

    bool dontContinue() { return false; }
}

TEST(EpollRunner, contructorCreatesEventPipeAndDestructorDeletesIt)
{
    int fd[2] = { 14, 15 };

    SystemMock sm;
    EXPECT_CALL(sm, pipe(_, O_CLOEXEC))
        .Times(1)
        .WillOnce(SetArrayArgument<0>(fd, fd + 2));
    EXPECT_CALL(sm, setCloseOnExec(_))
        .Times(AnyNumber());
    EXPECT_CALL(sm, setNonBlock(_))
        .Times(AnyNumber());
    EXPECT_CALL(sm, setNonBlock(14))
        .Times(1);
    EXPECT_CALL(sm, setNonBlock(15))
        .Times(0);

    EpollRunnerMock er(sm);

    EXPECT_CALL(sm, close(_))
        .Times(AnyNumber());
    EXPECT_CALL(sm, close(14))
        .Times(1);
    EXPECT_CALL(sm, close(15))
        .Times(1);
}

TEST(EpollRunner, contructorAddsEventPipesReadEndToEpollAndDestructorDeletesIt)
{
    int fd[2] = { 14, 15 };

    SystemMock sm;
    EXPECT_CALL(sm, pipe(_, O_CLOEXEC))
        .Times(1)
        .WillOnce(SetArrayArgument<0>(fd, fd + 2));
    EXPECT_CALL(sm, epollCtl(_, _, _, _))
        .Times(AnyNumber());
    EXPECT_CALL(sm, epollCtl(_, EPOLL_CTL_ADD, 14, _))
        .Times(1);

    EpollRunnerMock er(sm);

    EXPECT_CALL(sm, epollCtl(_, _, _, _))
        .Times(AnyNumber());
    EXPECT_CALL(sm, epollCtl(_, EPOLL_CTL_DEL, 14, _))
        .Times(1);
}

TEST(EpollRunner, dataInEventPipeIsRead)
{
    int fd[2] = { 14, 15 };

    struct epoll_event expectedEvents[1] = { };
    expectedEvents[0].events = EPOLLIN;
    expectedEvents[0].data.fd = 14;

    SystemMock sm;
    EXPECT_CALL(sm, pipe(_, O_CLOEXEC))
        .Times(1)
        .WillOnce(SetArrayArgument<0>(fd, fd + 2));

    EpollRunnerMock er(sm);

    EXPECT_CALL(sm, epollWait(_, _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArrayArgument<1>(expectedEvents, expectedEvents + 1),
                        Return(1)));
    EXPECT_CALL(sm, read(14, _, _))
        .Times(2)
        .WillOnce(Return(1))
        .WillOnce(Return(-1));

    er.run(dontContinue);
}

TEST(EpollRunner, addingCallbackFromSameThreadDoesntWriteToEventPipe)
{
    int fd[2] = { 14, 15 };

    SystemMock sm;
    EXPECT_CALL(sm, pipe(_, O_CLOEXEC))
        .Times(1)
        .WillOnce(SetArrayArgument<0>(fd, fd + 2));

    EpollRunnerMock er(sm);

    EXPECT_CALL(sm, write(15, _, _))
        .Times(0);

    er.addCallback(f);
}

namespace
{
    void *threadRunner(void *arg)
    {
        Runner *runner(static_cast<Runner *>(arg));
        usleep(200000);
        runner->addCallback(f);
        return nullptr;
    }
}

TEST(EpollRunner, addingCallbackFromAnotherThreadWritesToEventPipe)
{
    /*
     * I'm not sure how to fake timer addition from another thread, so using real
     * System class and real thread...
     */
    System s;
    EpollRunnerMock er(s);

    int status;
    pthread_t tid;
    status = pthread_create(&tid, nullptr, threadRunner, &er);
    ASSERT_FALSE(status);

    er.run(dontContinue);

    status = pthread_join(tid, nullptr);
    ASSERT_FALSE(status);
}

TEST(EpollRunner, canContinueIsCheckedIfThereWereCallbacks)
{
    EpollRunner runner;
    runner.addCallback(f);
    runner.run(dontContinue);
}

TEST(EpollRunner, callbacksCanBeUsedLikeCommandPattern)
{
    EpollRunner runner;
    bool executed(false);
    runner.addCallback([&runner, &executed] ()
                       {
                           runner.addCallback([&runner, &executed] ()
                                              {
                                                  runner.addCallback([&runner, &executed] ()
                                                                     {
                                                                         executed = true;
                                                                     });
                                              });
                       });
    runner.run(dontContinue);
    EXPECT_TRUE(executed);
}
