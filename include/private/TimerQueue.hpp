#ifndef EVENTLOOP_TIMERQUEUE_HEADER
#define EVENTLOOP_TIMERQUEUE_HEADER

#include <map>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <EventLoop/TimerPosition.hpp>
#include <EventLoop/Dumpable.hpp>
#include <EventLoop/Exception.hpp>

namespace EventLoop
{
    class System;
    class Timer;

    /**
     * TimerQueue is simple container for timers. The timers are kept in
     * execution order, so that the first timer to execute is always in front
     * of the queue and the second timer to execute is next in the queue and
     * so on.
     *
     * TimerQueue doesn't take ownership of the scheduled timers.
     */
    class TimerQueue: public Dumpable
    {
    public:
        TimerQueue();

        explicit TimerQueue(System &system);

        virtual ~TimerQueue();

        TimerQueue(const TimerQueue &) = delete;

        TimerQueue &operator = (const TimerQueue &) = delete;

        Timer *popFirstTimerToExecute();

        /** No timeout, equals -1 milliseconds. */
        static const boost::posix_time::time_duration NO_TIMEOUT;

        boost::posix_time::time_duration getTimeout() const;

        bool isEmpty() const;

        virtual void dump(std::ostream &os) const;

        void schedule(Timer                                     &timer,
                      const boost::posix_time::time_duration    &timeout);

        void cancel(const Timer &timer);

        void erase(TimerPosition i);

        TimerPosition end() { return queue.end(); }

        size_t getScheduledTimerCount() const { return queue.size(); }

    private:
        typedef std::multimap<boost::posix_time::time_duration, Timer *> Queue;
        System &system;
        Queue queue;
    };
}

#endif /* !EVENTLOOP_TIMERQUEUE_HEADER */
