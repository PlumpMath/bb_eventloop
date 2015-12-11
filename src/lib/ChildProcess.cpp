#include <EventLoop/ChildProcess.hpp>

#include <iostream>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <limits.h>

#include <EventLoop/Runner.hpp>
#include <EventLoop/DumpUtils.hpp>
#include <EventLoop/SystemException.hpp>

#include "private/System.hpp"
#include "private/SigPipeSuppressor.hpp"
#include "private/Abort.hpp"

using namespace EventLoop;

namespace
{
    void pipe(System            &system,
              FileDescriptor    &read,
              FileDescriptor    &write)
    {
        int fd[2] = { };
        system.pipe(fd, 0);
        read = fd[0];
        write = fd[1];
    }

    void changeFD(int   oldfd,
                  int   newfd)
    {
        if (oldfd != newfd)
        {
            ::dup2(oldfd, newfd);
            ::close(oldfd);
        }
    }

    void child(const ChildProcess::Args &args,
               int                      in,
               int                      out,
               int                      err) noexcept
    {
        changeFD(in, STDIN_FILENO);
        changeFD(out, STDOUT_FILENO);
        changeFD(err, STDERR_FILENO);
        std::vector<const char *> c;
        for (const auto &i : args)
        {
            c.push_back(i.c_str());
        }
        c.push_back(nullptr);
        execvp(args[0].c_str(), const_cast<char **>(c.data()));
        std::cerr << "exec(" << args[0] << "): " << strerror(errno) << std::endl;
        _exit(127);
    }
}

ChildProcess::ChildProcess(Runner           &runner,
                           const Args       &args,
                           std::istream     &in,
                           std::ostream     &out,
                           std::ostream     &err,
                           const SuccessCb  &successCb,
                           const ErrorCb    &errorCb,
                           const SignalCb   &signalCb):
    ChildProcess(System::getSystem(), runner, args, in, out, err, successCb, errorCb, signalCb) { }

ChildProcess::ChildProcess(System           &system,
                           Runner           &runner,
                           const Args       &args,
                           std::istream     &in,
                           std::ostream     &out,
                           std::ostream     &err,
                           const SuccessCb  &successCb,
                           const ErrorCb    &errorCb,
                           const SignalCb   &signalCb):
    system(system),
    runner(runner),
    in(system, in),
    out(system, out),
    err(system, err),
    pid(0),
    successCb(successCb),
    errorCb(errorCb),
    signalCb(signalCb)
{
    /*
     * The pipe parent writes to is blocking, but writing is done only when
     * there is EVENT_OUT and max PIPE_BUF bytes are written. This should
     * guarantee that we'll never block on write and we never have to push
     * unwritten bytes back to 'in.is'.
     */
    FileDescriptor inRead(system);
    pipe(system, inRead, this->in.fd);
    system.setCloseOnExec(this->in.fd);
    runner.addFDListener(*this, this->in.fd, FDListener::EVENT_OUT);

    FileDescriptor outWrite(system);
    pipe(system, this->out.fd, outWrite);
    system.setCloseOnExec(this->out.fd);
    system.setNonBlock(this->out.fd);
    runner.addFDListener(*this, this->out.fd, FDListener::EVENT_IN);

    FileDescriptor errWrite(system);
    pipe(system, this->err.fd, errWrite);
    system.setCloseOnExec(this->err.fd);
    system.setNonBlock(this->err.fd);
    runner.addFDListener(*this, this->err.fd, FDListener::EVENT_IN);

    pid = system.fork();
    if (pid == 0)
    {
        try
        {
            child(args, inRead, outWrite, errWrite);
        }
        catch (...)
        {
        }
        _exit(126);
    }

    inRead.close();
    outWrite.close();
    errWrite.close();
}

ChildProcess::~ChildProcess()
{
    kill();
}

void ChildProcess::kill()
{
    if (pid != 0)
    {
        system.kill(pid, SIGKILL);
        int tmp(0);
        system.waitpid(pid, &tmp, 0);
    }
    closeAll();
}

void ChildProcess::sigChildReceived()
{
    if (pid != 0)
    {
        int status(0);
        if (system.waitpid(pid, &status, WNOHANG) == pid)
        {
            pid = 0;
            closeAll();
            handleWaitStatus(status);
        }
    }
}

void ChildProcess::handleWaitStatus(int status)
{
    if (WIFEXITED(status))
    {
        const int exitStatus(WEXITSTATUS(status));
        if ((exitStatus == 0) &&
            (successCb))
        {
            successCb();
        }
        else if ((status != 0) &&
                 (errorCb))
        {
            errorCb(exitStatus);
        }
    }
    else if (WIFSIGNALED(status))
    {
        if (signalCb)
        {
            signalCb(WTERMSIG(status));
        }
    }
    else
    {
        EVENTLOOP_ABORT();
    }
}

void ChildProcess::closeAll()
{
    close(in.fd);
    close(out.fd);
    close(err.fd);
}

void ChildProcess::close(FileDescriptor &fd)
{
    if (fd)
    {
        runner.delFDListener(fd);
        fd.close();
    }
}

void ChildProcess::handleEventsCb(int           fd,
                                  unsigned int  events)
{
    if (events & FDListener::EVENT_IN)
    {
        handleEventIn(fd);
    }
    if (events & FDListener::EVENT_OUT)
    {
        handleEventOut();
    }
    if (events & FDListener::EVENT_HUP)
    {
        handleEventHup(fd);
    }
}

void ChildProcess::handleEventIn(int fd)
{
    Input &input(getInput(fd));
    char buffer[PIPE_BUF];
    ssize_t ssize;
    do
    {
        ssize = system.read(input.fd, buffer, sizeof(buffer));
        if (ssize > 0)
        {
            input.os.write(buffer, ssize);
        }
    }
    while (ssize > 0);
}

void ChildProcess::handleEventOut()
{
    if (in.is)
    {
        char buffer[PIPE_BUF];
        in.is.read(buffer, sizeof(buffer));
        const size_t toWrite(in.is.gcount());
        try
        {
            SigPipeSuppressor spb(system);
            const ssize_t ssize(system.write(in.fd, buffer, toWrite));
            assert(static_cast<size_t>(ssize) == toWrite);
        }
        catch (const SystemException &)
        {
            close(in.fd);
        }
    }
    else
    {
        close(in.fd);
    }
}

void ChildProcess::handleEventHup(int fd)
{
    if (fd == out.fd)
    {
        close(out.fd);
    }
    else if (fd == err.fd)
    {
        close(err.fd);
    }
    else if (fd == in.fd)
    {
        close(in.fd);
    }
}

ChildProcess::Input &ChildProcess::getInput(int fd)
{
    if (out.fd == fd)
    {
        return out;
    }
    else if (err.fd == fd)
    {
        return err;
    }
    EVENTLOOP_ABORT();
}

void ChildProcess::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, system);
    DUMP_ADDRESS_OF(os, runner);
    DUMP_VALUE_OF(os, in.fd);
    DUMP_ADDRESS_OF(os, in.is);
    DUMP_VALUE_OF(os, out.fd);
    DUMP_ADDRESS_OF(os, out.os);
    DUMP_VALUE_OF(os, err.fd);
    DUMP_ADDRESS_OF(os, err.os);
    DUMP_VALUE_OF(os, pid);
    DUMP_VALUE_OF(os, successCb);
    DUMP_VALUE_OF(os, errorCb);
    DUMP_VALUE_OF(os, signalCb);
    DUMP_AS(os, FDListener);
    DUMP_END(os);
}
