#ifndef EVENTLOOP_CHILDPROCESS_HEADER
#define EVENTLOOP_CHILDPROCESS_HEADER

#include <vector>
#include <string>
#include <functional>
#include <iosfwd>
#include <sys/types.h>
#include <unistd.h>

#include <EventLoop/FDListener.hpp>
#include <EventLoop/FileDescriptor.hpp>

namespace EventLoop
{
    class System;
    class Runner;

    /**
     * Asynchronous child process with input and output.
     */
    class ChildProcess: public FDListener
    {
    public:
        typedef std::vector<std::string> Args;

        typedef std::function<void()> SuccessCb;

        typedef std::function<void(int)> ErrorCb;

        typedef std::function<void(int)> SignalCb;

        ChildProcess() = delete;

        ChildProcess(const ChildProcess &) = default;

        ChildProcess &operator = (const ChildProcess &) = default;

        /**
         * Fork a new child process to execute the given args. The given
         * input stream is fed to the child process' standard input and
         * the child process' standard output is gathered to the given
         * output stream and standard error is gathered to the given
         * error stream. All this happens asynchronously on the background.
         *
         * Once the child process dies, one of the given callback functions
         * will be called depending on how the child died.
         *
         * @param args      Arguments to execute, for example
         *                  { "echo", "hello world" }.
         * @param in        The input stream to use as the child's standard
         *                  input.
         * @param out       The output stream to use as the child's standard
         *                  output.
         * @param err       The error stream to use as the child's standard
         *                  error.
         * @param successCb Callback function to be called if the child exits
         *                  without error.
         * @param errorCb   Callback function to be called if the child exits
         *                  with error.
         * @param signalCb  Callback function to be called if the child is
         *                  killed with signal.
         */
        ChildProcess(Runner             &runner,
                     const Args         &args,
                     std::istream       &in,
                     std::ostream       &out,
                     std::ostream       &err,
                     const SuccessCb    &successCb,
                     const ErrorCb      &errorCb,
                     const SignalCb     &signalCb);

        /**
         * Kill the child and close all pipes towards the child, if not
         * already killed or closed.
         */
        virtual ~ChildProcess();

        /**
         * Kill the child and close all pipes towards the child, if not
         * already killed or closed.
         */
        void kill();

        /**
         * Function to be called when parent process receives SIGCHLD.
         * Signal handling is not done by ChildProcess class, but is left
         * for the application (and can be done with for example EpollRunner).
         */
        void sigChildReceived();

        virtual void dump(std::ostream &os) const;

    protected:
        ChildProcess(System             &system,
                     Runner             &runner,
                     const Args         &args,
                     std::istream       &in,
                     std::ostream       &out,
                     std::ostream       &err,
                     const SuccessCb    &successCb,
                     const ErrorCb      &errorCb,
                     const SignalCb     &signalCb);

    private:
        struct Input
        {
            Input() = delete;

            Input(System &system, std::ostream &os): fd(system), os(os) { }

            Input(const Input &) = delete;

            Input &operator = (const Input &) = delete;

            FileDescriptor fd;
            std::ostream &os;
        };

        struct Output
        {
            Output() = delete;

            Output(System &system, std::istream &is): fd(system), is(is) { }

            Output(const Output &) = delete;

            Output &operator = (const Output &) = delete;

            FileDescriptor fd;
            std::istream &is;
        };

        void closeAll();

        void close(FileDescriptor &fd);

        void handleEventsCb(int             fd,
                            unsigned int    events);

        void handleEventIn(int fd);

        void handleEventOut();

        void handleEventHup(int fd);

        Input &getInput(int fd);

        void handleWaitStatus(int status);

        System &system;
        Runner &runner;
        Output in;
        Input out;
        Input err;
        pid_t pid;
        SuccessCb successCb;
        ErrorCb errorCb;
        SignalCb signalCb;
    };
}

#endif /* !EVENTLOOP_CHILDPROCESS_HEADER */
