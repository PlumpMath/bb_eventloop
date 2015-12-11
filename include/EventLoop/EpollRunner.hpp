#ifndef EVENTLOOP_EPOLLRUNNER_HEADER
#define EVENTLOOP_EPOLLRUNNER_HEADER

#include <functional>
#include <memory>
#include <boost/thread/thread.hpp>

#include <EventLoop/Runner.hpp>
#include <EventLoop/Exception.hpp>

namespace EventLoop
{
    class System;
    class SignalSet;
    class BaseEpollRunner;
    class FDMonitor;

    /**
     * Runner using epoll().
     */
    class EpollRunner: public Runner
    {
    public:
        class NoSignalHandling: public Exception
        {
        public:
            NoSignalHandling(): Exception("EpollRunner is not handling signals") { }
        };

        typedef std::function<void(int)> SignalHandler;

        /**
         * Create runner without signal handling.
         */
        EpollRunner();

        /**
         * Create runner with signal handling.
         *
         * The passed signals are blocked during normal execution (meaning
         * timers, file descriptor handling and callbacks) and handled only
         * when waiting for new events. Note that child process created with
         * fork(2) and execve(2) inherit the same signal mask of the thread
         * calling execve(2).
         *
         * @param signalSet     Signals to catch.
         * @param signalHandler Signal handler function to call when signals
         *                      are caught.
         */
        EpollRunner(const SignalSet &signalSet,
                    SignalHandler   signalHandler);

        virtual ~EpollRunner();

        typedef std::function<bool()> CanContinue;

        /**
         * Run the event-loop logic until exception is thrown or the given
         * canContinue function returns false. The given canContinue function
         * is called after every loop.
         */
        void run(CanContinue canContinue);

        /**
         * Run the event-loop logic until exception is thrown.
         */
        void run() { run(doContinue); }

        /**
         * Replace the signal set given in constructor with the given new
         * signal set.
         *
         * @param signalSet Signals to catch.
         *
         * @exception NoSignalHandling if signal handling has not been set
         *            in constructor.
         */
        void setSignals(const SignalSet &signalSet);

        void addFDListener(FDListener   &fl,
                           int          fd,
                           unsigned int events);

        void modifyFDListener(int           fd,
                              unsigned int  events);

        void delFDListener(int fd);

        void addCallback(const std::function<void()> &cb);

        virtual void dump(std::ostream &os) const;

    protected:
        explicit EpollRunner(System &system);

        EpollRunner(System          &system,
                    const SignalSet &signalSet,
                    SignalHandler   signalHandler);

    private:
        TimerQueue &getTimerQueue();

        static bool doContinue() { return true; }

        class EventPipe;
        class SignalFD;

        static bool isTimeout(const boost::posix_time::time_duration &timeout);

        void runImpl(CanContinue canContinue);

        bool wait(const boost::posix_time::time_duration &timeout);

        void wakeUp();

        System &system;
        std::unique_ptr<FDMonitor> fdsDeletedWhileHandlingEvents;
        std::unique_ptr<BaseEpollRunner> baseEpollRunner;
        std::unique_ptr<EventPipe> eventPipe;
        std::unique_ptr<SignalFD> signalFD;
        boost::thread::id runnerThreadId;
    };
}

#endif /* !EVENTLOOP_EPOLLRUNNER_HEADER */
