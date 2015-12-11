#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <pthread.h>
#include <gtest/gtest.h>

#include <EventLoop/Timer.hpp>
#include <EventLoop/SignalSet.hpp>

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

bool operator == (const struct sigaction &a, const struct sigaction &b) { return memcmp(&a, &b, sizeof(a)) == 0; }

bool operator == (const sigset_t &a, const sigset_t &b) { return memcmp(&a, &b, sizeof(a)) == 0; }

namespace
{
    bool dontContinue() { return false; }

    class EpollRunnerWithSignals: public testing::Test
    {
    public:
        SystemMock sm;
        SignalSet ss;
        struct sigaction termAction;
        struct sigaction hupAction;
        std::vector<int> handledSignals;
        std::unique_ptr<EpollRunnerMock> er;
        const int SIGNAL_FD;

        EpollRunnerWithSignals():
            ss({ SIGTERM, SIGHUP }),
            SIGNAL_FD(4567)
        {
            memset(&termAction, 0, sizeof(termAction));
            termAction.sa_handler = (void(*)(int))(123);
            memset(&hupAction, 0, sizeof(hupAction));
            hupAction.sa_handler = (void(*)(int))(456);
        }

        void signalHandler(int sig)
        {
            handledSignals.push_back(sig);
        }

        void createER()
        {
            int efd[2] = { 14, 15 };

            ON_CALL(sm, getMonotonicClock())
                .WillByDefault(Return(boost::posix_time::milliseconds(0)));
            EXPECT_CALL(sm, pipe(_, O_CLOEXEC))
                .Times(1)
                .WillOnce(SetArrayArgument<0>(efd, efd + 2));
            EXPECT_CALL(sm, sigMask(SIG_BLOCK, NotNull(), NotNull()))
                .Times(1)
                .WillOnce(Invoke([] (int, const sigset_t *, sigset_t *oldset)
                                 {
                                     /* Pretend that SIGUSR2 was already blocked */
                                     sigaddset(oldset, SIGUSR2);
                                 }));
            EXPECT_CALL(sm, signalFD(-1, NotNull(), SFD_NONBLOCK | SFD_CLOEXEC))
                .Times(1)
                .WillOnce(Return(SIGNAL_FD));

            er.reset(new EpollRunnerMock(sm, ss, std::bind(&EpollRunnerWithSignals::signalHandler, this, std::placeholders::_1)));
            Mock::VerifyAndClear(&sm);
        }

        void destroyER()
        {
            /* Check that the original mask of SIGUSR2 is restored */
            EXPECT_CALL(sm, sigMask(SIG_SETMASK, Pointee(SignalSet(SIGUSR2).getSet()), IsNull()))
                .Times(1);
            er.reset();
            Mock::VerifyAndClear(&sm);
        }

        void readSig(int, void *ptr, size_t)
        {
            struct signalfd_siginfo *siginfo(static_cast<struct signalfd_siginfo *>(ptr));
            memset(siginfo, 0, sizeof(*siginfo));
            siginfo->ssi_signo = SIGTERM;
        }
    };

    class CreatedEpollRunnerWithSignals: public EpollRunnerWithSignals
    {
    public:
        CreatedEpollRunnerWithSignals() { createER(); }

        ~CreatedEpollRunnerWithSignals() { destroyER(); }
    };
}

TEST_F(EpollRunnerWithSignals, constructorSetsUpSignalHandlingDestructorUnsets)
{
    createER();
    destroyER();
}

TEST_F(EpollRunnerWithSignals, signalsAreHandledAsCallbacksOutSideTheFDHandling)
{
    createER();

    struct epoll_event expectedEvents[1] = { };
    expectedEvents[0].events = EPOLLIN;
    expectedEvents[0].data.fd = SIGNAL_FD;

    EXPECT_CALL(sm, epollWait(_, _, _, -1))
        .Times(1)
        .WillOnce(DoAll(SetArrayArgument<1>(expectedEvents, expectedEvents + 1),
                        Return(1)));
    EXPECT_CALL(sm, read(SIGNAL_FD, _, sizeof(struct signalfd_siginfo)))
        .Times(2)
        .WillOnce(DoAll(Invoke(this, &EpollRunnerWithSignals::readSig),
                        Return(1)))
        .WillOnce(Return(-1));

    er->run(dontContinue); /* The first round handles signal fd */
    er->run(dontContinue); /* The second round handles callbacks */

    EXPECT_THAT(handledSignals, ElementsAre(SIGTERM));

    destroyER();
}

TEST_F(CreatedEpollRunnerWithSignals, setSignalsReplacesTheOldSignalSetWithTheNewGivenOne)
{
    InSequence dummy;
    /*
     * Old signals were SIGTERM, SIGHUP
     * New signals are SIGINT, SIGHUP
     */
    EXPECT_CALL(sm, sigMask(SIG_BLOCK, Pointee(SignalSet(SIGINT).getSet()), IsNull()))
        .Times(1);
    EXPECT_CALL(sm, sigMask(SIG_UNBLOCK, Pointee(SignalSet(SIGTERM).getSet()), IsNull()))
        .Times(1);
    EXPECT_CALL(sm, signalFD(SIGNAL_FD, Pointee(SignalSet({SIGINT, SIGHUP}).getSet()), 0))
        .Times(1)
        .WillOnce(Return(SIGNAL_FD));
    er->setSignals(SignalSet({SIGINT, SIGHUP}));
    Mock::VerifyAndClear(&sm);
}

TEST_F(CreatedEpollRunnerWithSignals, originallyBlockedSignalsAreNeverUnblocked)
{
    InSequence dummy;
    /*
     * Old signals were SIGTERM, SIGHUP
     * New signals are SIGUSR2, SIGHUP
     * SIGUSR2 was already blocked when EpollRunner was created
     */
    EXPECT_CALL(sm, sigMask(SIG_BLOCK, Pointee(SignalSet().getSet()), IsNull()))
        .Times(1);
    EXPECT_CALL(sm, sigMask(SIG_UNBLOCK, Pointee(SignalSet(SIGTERM).getSet()), IsNull()))
        .Times(1);
    EXPECT_CALL(sm, signalFD(SIGNAL_FD, Pointee(SignalSet({SIGUSR2, SIGHUP}).getSet()), 0))
        .Times(1)
        .WillOnce(Return(SIGNAL_FD));
    er->setSignals(SignalSet({SIGUSR2, SIGHUP}));
    Mock::VerifyAndClear(&sm);

    /* Stop signal handling, orginally blocked SIGUSR2 must not be unblocked */
    EXPECT_CALL(sm, sigMask(SIG_BLOCK, Pointee(SignalSet().getSet()), IsNull()))
        .Times(1);
    EXPECT_CALL(sm, sigMask(SIG_UNBLOCK, Pointee(SignalSet(SIGHUP).getSet()), IsNull()))
        .Times(1);
    EXPECT_CALL(sm, signalFD(SIGNAL_FD, Pointee(SignalSet().getSet()), 0))
        .Times(1)
        .WillOnce(Return(SIGNAL_FD));
    er->setSignals(SignalSet());
    Mock::VerifyAndClear(&sm);
}

TEST_F(CreatedEpollRunnerWithSignals, dump)
{
    EXPECT_CALL(sm, sigMask(SIG_BLOCK, IsNull(), NotNull()))
        .Times(1);
    EXPECT_VALID_DUMP(*er);
}
