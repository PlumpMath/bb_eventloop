#ifndef EVENTLOOP_RUNNER_HEADER
#define EVENTLOOP_RUNNER_HEADER

#include <functional>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <EventLoop/TimerPosition.hpp>
#include <EventLoop/Dumpable.hpp>
#include <EventLoop/Exception.hpp>

namespace EventLoop
{
    class FDListener;
    class Timer;
    class TimerQueue;

    /**
     * Runner is the heart of EventLoop. For concrete implemenations of Runner
     * see EpollRunner and AsyncRunner.
     */
    class Runner: public Dumpable
    {
        friend class Timer;

    public:
        class NullCallback: public Exception
        {
        public:
            NullCallback(): Exception("null callback") { }
        };

        class FDAlreadyAdded: public Exception
        {
        public:
            FDAlreadyAdded() = delete;

            explicit FDAlreadyAdded(int fd);
        };

        class NonExistingFDListener: public Exception
        {
        public:
            NonExistingFDListener();

            explicit NonExistingFDListener(int fd);
        };

        Runner();

        virtual ~Runner();

        Runner(const Runner &) = delete;

        Runner &operator = (const Runner &) = delete;

        /**
         * Add new file descriptor with associated FDListener and events.
         *
         * @exception FDAlreadyAdded if the file descriptor has already been
         *            added.
         */
        virtual void addFDListener(FDListener   &fl,
                                   int          fd,
                                   unsigned int events) = 0;

        /**
         * Modify existing file descriptor to handle the given new events.
         *
         * @exception NonExistingFDListener if the file descriptor has not been
         *            added.
         */
        virtual void modifyFDListener(int           fd,
                                      unsigned int  events) = 0;

        /**
         * Delete existing file descriptor.
         *
         * @exception NonExistingFDListener if the file descriptor has not been
         *            added.
         */
        virtual void delFDListener(int fd) = 0;

        /**
         * Add a new callback to be executed at the beginning of next loop
         * iteration.
         *
         * The given callback can be seen as one way to implement Command
         * pattern (http://en.wikipedia.org/wiki/Command_pattern) but
         * without the Command class.
         *
         * @param cb    Callback to add.
         *
         * @exception NullCallback if the given cb is nullptr.
         *
         * @note This is the only thread-safe function of Runner and can be used
         *       for pushing work from another thread to Runner thread.
         */
        virtual void addCallback(const std::function<void()> &cb) = 0;

        virtual void dump(std::ostream &os) const = 0;

    protected:
        virtual TimerQueue &getTimerQueue() = 0;
    };
}

#endif /* !EVENTLOOP_RUNNER_HEADER */
