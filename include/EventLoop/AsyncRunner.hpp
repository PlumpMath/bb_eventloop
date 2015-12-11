#ifndef EVENTLOOP_ASYNCRUNNER_HEADER
#define EVENTLOOP_ASYNCRUNNER_HEADER

#include <memory>

#include <EventLoop/Runner.hpp>

namespace EventLoop
{
    class System;
    class BaseEpollRunner;
    class FDMonitor;

    /**
     * Asynchronous runner provides possibility for some other event loop
     * implementation to take care of monitoring events with select/poll/epoll
     * or whatever implementation by providing single file descriptor to
     * monitor and single function to call when some event was seen in the
     * file descriptor.
     */
    class AsyncRunner: public Runner
    {
    public:
        AsyncRunner();

        virtual ~AsyncRunner();

        /**
         * Execute one loop of the event-loop logic without blocking. This
         * function is expected to be called after EVENT_IN happens in the
         * file descriptor returned by the getEventFD() function.
         */
        void step();

        /**
         * Get the file descriptor to monitor for any EVENT_IN.
         */
        int getEventFD() const;

        void addFDListener(FDListener   &fl,
                           int          fd,
                           unsigned int events);

        void modifyFDListener(int           fd,
                              unsigned int  events);

        void delFDListener(int fd);

        void addCallback(const std::function<void()> &cb);

        virtual void dump(std::ostream &os) const;

    protected:
        explicit AsyncRunner(System &system);

    private:
        TimerQueue &getTimerQueue();

        class TimerFD;

        void waitAndHandleFDEvents();

        void executeTimersAndUpdateTimer();

        std::unique_ptr<FDMonitor> fdsDeletedWhileHandlingEvents;
        std::unique_ptr<BaseEpollRunner> baseEpollRunner;
        std::unique_ptr<TimerFD> timerFD;
    };
}

#endif /* !EVENTLOOP_ASYNCRUNNER_HEADER */
