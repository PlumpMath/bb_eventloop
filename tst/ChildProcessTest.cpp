#include <memory>
#include <iostream>
#include <sstream>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/SignalSet.hpp>
#include <EventLoop/EpollRunner.hpp>
#include <EventLoop/ChildProcess.hpp>

#include "private/tst/DummyFDListenerMap.hpp"
#include "private/tst/SystemMock.hpp"
#include "private/tst/DumpTest.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    class ChildProcessHelper: public ChildProcess
    {
    public:
        ChildProcessHelper(System           &system,
                           Runner           &runner,
                           const Args       &args,
                           std::istream     &in,
                           std::ostream     &out,
                           std::ostream     &err):
            ChildProcess(system,
                         runner,
                         args,
                         in,
                         out,
                         err,
                         std::bind(&ChildProcessHelper::successCb, this),
                         std::bind(&ChildProcessHelper::errorCb, this, std::placeholders::_1),
                         std::bind(&ChildProcessHelper::signalCb, this, std::placeholders::_1)) { }

        MOCK_METHOD0(successCb, void());

        MOCK_METHOD1(errorCb, void(int));

        MOCK_METHOD1(signalCb, void(int));
    };

    class ChildProcessTest: public testing::Test
    {
    public:
        SystemMock sm;
        DummyFDListenerMap flm;
        std::stringstream in;
        std::stringstream out;
        std::stringstream err;
        const int inRead;
        const int inWrite;
        const int outRead;
        const int outWrite;
        const int errRead;
        const int errWrite;
        const pid_t pid;
        std::unique_ptr<ChildProcessHelper> cp;

        ChildProcessTest():
            inRead(111),
            inWrite(222),
            outRead(333),
            outWrite(444),
            errRead(555),
            errWrite(666),
            pid(777)
        {
            ON_CALL(sm, fork())
                .WillByDefault(Return(pid));
        }

        void create(const ChildProcess::Args &args)
        {
            InSequence dummy;

            EXPECT_CALL(sm, pipe(_, 0))
                .Times(1)
                .WillOnce(Invoke([this] (int fd[2], int) { fd[0] = inRead; fd[1] = inWrite; }));
            EXPECT_CALL(sm, setCloseOnExec(inWrite))
                .Times(1);
            EXPECT_CALL(sm, setNonBlock(inWrite))
                .Times(0);

            EXPECT_CALL(sm, pipe(_, 0))
                .Times(1)
                .WillOnce(Invoke([this] (int fd[2], int) { fd[0] = outRead; fd[1] = outWrite; }));
            EXPECT_CALL(sm, setCloseOnExec(outRead))
                .Times(1);
            EXPECT_CALL(sm, setNonBlock(outRead))
                .Times(1);

            EXPECT_CALL(sm, pipe(_, 0))
                .Times(1)
                .WillOnce(Invoke([this] (int fd[2], int) { fd[0] = errRead; fd[1] = errWrite; }));
            EXPECT_CALL(sm, setCloseOnExec(errRead))
                .Times(1);
            EXPECT_CALL(sm, setNonBlock(errRead))
                .Times(1);

            EXPECT_CALL(sm, fork())
                .Times(1)
                .WillOnce(Return(pid));

            EXPECT_CALL(sm, close(inRead))
                .Times(1);
            EXPECT_CALL(sm, close(outWrite))
                .Times(1);
            EXPECT_CALL(sm, close(errWrite))
                .Times(1);

            cp.reset(new ChildProcessHelper(sm, flm, args, in, out, err));
            Mock::VerifyAndClearExpectations(&sm);
        }

        void destroy()
        {
            InSequence dummy;

            EXPECT_CALL(sm, kill(pid, SIGKILL))
                .Times(1);
            EXPECT_CALL(sm, waitpid(pid, Ne(nullptr), 0))
                .Times(1);
            EXPECT_CALL(sm, close(inWrite))
                .Times(1);
            EXPECT_CALL(sm, close(outRead))
                .Times(1);
            EXPECT_CALL(sm, close(errRead))
                .Times(1);

            cp.reset(nullptr);
        }
    };

    class CreatedChildProcessTest: public ChildProcessTest
    {
    public:
        CreatedChildProcessTest()
        {
            in << "testmessage";
            create(ChildProcess::Args({ "echo", "helloworld" }));
        }

        ~CreatedChildProcessTest()
        {
            cp.reset();
        }
    };
}

TEST_F(ChildProcessTest, constructorForksNewChildAndSetsUpPipesAndDestructorKillsChildAndClosesPipes)
{
    create(ChildProcess::Args({ "echo", "helloworld" }));

    EXPECT_EQ(FDListener::EVENT_OUT, flm.getEvents(inWrite));
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(outRead));
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(errRead));

    destroy();

    EXPECT_THROW(flm.getEvents(inWrite), Runner::NonExistingFDListener);
    EXPECT_THROW(flm.getEvents(outRead), Runner::NonExistingFDListener);
    EXPECT_THROW(flm.getEvents(errRead), Runner::NonExistingFDListener);
}

TEST_F(CreatedChildProcessTest, outputFromChildIsReadAndAppendedToOutputStreamGivenInConstructor)
{
    InSequence dummy;
    const std::string msg("foobar rules!");
    EXPECT_CALL(sm, read(outRead, Ne(nullptr), Gt(msg.size())))
        .Times(2)
        .WillOnce(Invoke([this, &msg] (int, void *buf, size_t)
                         {
                             memcpy(buf, msg.data(), msg.size());
                             return msg.size();
                         }))
        .WillOnce(Return(-1));
    cp->handleEvents(outRead, FDListener::EVENT_IN);
    EXPECT_EQ(msg, out.str());
}

TEST_F(CreatedChildProcessTest, noDataInOutputDoesntChangeTheOutputStreamGivenInConstructor)
{
    InSequence dummy;
    EXPECT_CALL(sm, read(outRead, Ne(nullptr), Gt(0U)))
        .Times(1)
        .WillOnce(Return(-1));
    cp->handleEvents(outRead, FDListener::EVENT_IN);
    EXPECT_EQ("", out.str());
}

TEST_F(CreatedChildProcessTest, ifChildClosesOutputParentStopsReadingIt)
{
    InSequence dummy;
    EXPECT_CALL(sm, close(outRead))
        .Times(1);
    cp->handleEvents(outRead, FDListener::EVENT_HUP);
    Mock::VerifyAndClearExpectations(&sm);
    EXPECT_THROW(flm.getEvents(outRead), Runner::NonExistingFDListener);
}

TEST_F(CreatedChildProcessTest, errorFromChildIsReadAndAppendedToErrorStreamGivenInConstructor)
{
    InSequence dummy;
    const std::string msg("foobar rules!");
    EXPECT_CALL(sm, read(errRead, Ne(nullptr), Gt(msg.size())))
        .Times(2)
        .WillOnce(Invoke([this, &msg] (int, void *buf, size_t)
                         {
                             memcpy(buf, msg.data(), msg.size());
                             return msg.size();
                         }))
        .WillOnce(Return(-1));
    cp->handleEvents(errRead, FDListener::EVENT_IN);
    EXPECT_EQ(msg, err.str());
}

TEST_F(CreatedChildProcessTest, ifChildClosesErrorParentStopsReadingIt)
{
    InSequence dummy;
    EXPECT_CALL(sm, close(errRead))
        .Times(1);
    cp->handleEvents(errRead, FDListener::EVENT_HUP);
    Mock::VerifyAndClearExpectations(&sm);
    EXPECT_THROW(flm.getEvents(errRead), Runner::NonExistingFDListener);
}

TEST_F(CreatedChildProcessTest, whenChildCanReadTheInputStreamGivenInConstructorIsSentToChild)
{
    InSequence dummy;
    EXPECT_CALL(sm, write(inWrite, Ne(nullptr), Gt(0U)))
        .Times(1)
        .WillOnce(ReturnArg<2>());
    cp->handleEvents(inWrite, FDListener::EVENT_OUT);
}

TEST_F(CreatedChildProcessTest, onceInputStreamIsSentToChildTheInputIsClosed)
{
    InSequence dummy;
    EXPECT_CALL(sm, write(inWrite, Ne(nullptr), Gt(0U)))
        .Times(1)
        .WillOnce(ReturnArg<2>());
    cp->handleEvents(inWrite, FDListener::EVENT_OUT);
    EXPECT_CALL(sm, close(inWrite))
        .Times(1);
    cp->handleEvents(inWrite, FDListener::EVENT_OUT);
    Mock::VerifyAndClearExpectations(&sm);
    EXPECT_THROW(flm.getEvents(inWrite), Runner::NonExistingFDListener);
}

TEST_F(CreatedChildProcessTest, ifChildClosesInputParentStopsWritingIt)
{
    InSequence dummy;
    EXPECT_CALL(sm, close(inWrite))
        .Times(1);
    cp->handleEvents(inWrite, FDListener::EVENT_HUP);
    Mock::VerifyAndClearExpectations(&sm);
    EXPECT_THROW(flm.getEvents(inWrite), Runner::NonExistingFDListener);
}

TEST_F(CreatedChildProcessTest, errorWhileWritingToChildClosesTheWritingPipe)
{
    InSequence dummy;
    EXPECT_CALL(sm, write(inWrite, Ne(nullptr), Gt(0U)))
        .Times(1)
        .WillOnce(Throw(SystemException("write")));
    EXPECT_CALL(sm, close(inWrite))
        .Times(1);
    cp->handleEvents(inWrite, FDListener::EVENT_OUT);
    Mock::VerifyAndClearExpectations(&sm);
    EXPECT_THROW(flm.getEvents(inWrite), Runner::NonExistingFDListener);
}

TEST_F(CreatedChildProcessTest, sigChildReceivedChecksIfTheChildIsDead)
{
    InSequence dummy;
    EXPECT_CALL(sm, close(_))
        .Times(0);
    EXPECT_CALL(sm, waitpid(pid, Ne(nullptr), WNOHANG))
        .Times(1)
        .WillOnce(Return(0));
    cp->sigChildReceived();
    Mock::VerifyAndClearExpectations(&sm);
}

TEST_F(CreatedChildProcessTest, ifChildDiesAllPipesAreClosed)
{
    InSequence dummy;
    EXPECT_CALL(sm, waitpid(pid, Ne(nullptr), WNOHANG))
        .Times(1)
        .WillOnce(Return(pid));
    EXPECT_CALL(sm, close(inWrite))
        .Times(1);
    EXPECT_CALL(sm, close(outRead))
        .Times(1);
    EXPECT_CALL(sm, close(errRead))
        .Times(1);
    cp->sigChildReceived();
    Mock::VerifyAndClearExpectations(&sm);
    EXPECT_THROW(flm.getEvents(inWrite), Runner::NonExistingFDListener);
    EXPECT_THROW(flm.getEvents(outRead), Runner::NonExistingFDListener);
    EXPECT_THROW(flm.getEvents(errRead), Runner::NonExistingFDListener);

    /* waitpid is not done again */
    EXPECT_CALL(sm, waitpid(_, _, _))
        .Times(0);
    cp->sigChildReceived();
    Mock::VerifyAndClearExpectations(&sm);
}

TEST_F(CreatedChildProcessTest, successfullyExitedChildIsNotifiedWithSuccessCb)
{
    InSequence dummy;
    EXPECT_CALL(sm, waitpid(pid, Ne(nullptr), WNOHANG))
        .Times(1)
        .WillOnce(Invoke([this] (pid_t, int *s, int)
                         {
                             union wait w = { };
                             w.w_retcode = 0;
                             *s = w.w_status;
                             return pid;
                         }));
    EXPECT_CALL(*cp, successCb())
        .Times(1);
    cp->sigChildReceived();
    Mock::VerifyAndClearExpectations(&sm);
}

TEST_F(CreatedChildProcessTest, unsuccessfullyExitedChildIsNotifiedWithErrorCb)
{
    const int status(11);
    InSequence dummy;
    EXPECT_CALL(sm, waitpid(pid, Ne(nullptr), WNOHANG))
        .Times(1)
        .WillOnce(Invoke([this, status] (pid_t, int *s, int)
                         {
                             union wait w = { };
                             w.w_retcode = status;
                             *s = w.w_status;
                             return pid;
                         }));
    EXPECT_CALL(*cp, errorCb(status))
        .Times(1);
    cp->sigChildReceived();
    Mock::VerifyAndClearExpectations(&sm);
}

TEST_F(CreatedChildProcessTest, childKilledWithSignalIsNotifiedWithSignalCb)
{
    const int sig(SIGTERM);
    InSequence dummy;
    EXPECT_CALL(sm, waitpid(pid, Ne(nullptr), WNOHANG))
        .Times(1)
        .WillOnce(Invoke([this, sig] (pid_t, int *s, int)
                         {
                             union wait w = { };
                             w.w_termsig = sig;
                             *s = w.w_status;
                             return pid;
                         }));
    EXPECT_CALL(*cp, signalCb(sig))
        .Times(1);
    cp->sigChildReceived();
    Mock::VerifyAndClearExpectations(&sm);
}

TEST_F(CreatedChildProcessTest, dump)
{
    EXPECT_VALID_DUMP(*cp);
}
