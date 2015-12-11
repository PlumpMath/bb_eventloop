#ifndef EVENTLOOP_TIMER_HEADER
#define EVENTLOOP_TIMER_HEADER

#include <functional>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <EventLoop/TimerPosition.hpp>
#include <EventLoop/Dumpable.hpp>
#include <EventLoop/Exception.hpp>

namespace EventLoop
{
    class Runner;
    class TimerQueue;

    /**
     * Timer is base class for generic one-shot timers. Timers are held in
     * Runner and executed after the timeout has expired, unless first
     * canceled or destroyed.
     */
    class Timer: public Dumpable
    {
        friend class TimerQueue;

    public:
        /** Exception for null callback. */
        class NullCallback: public Exception
        {
        public:
            NullCallback(): Exception("null callback") { }
        };

        /** Type of callback functions. */
        typedef std::function<void()> Callback;

        Timer() = delete;

        /**
         * Create timer, but do not schedule yet.
         *
         * @param runner The Runner to use.
         *
         * @note The passed Runner must not be destroyed before this
         *       timer is destroyed.
         */
        explicit Timer(Runner &runner);

        /**
         * Cancel this timer if scheduled and destroy.
         */
        virtual ~Timer();

        Timer(const Timer &) = delete;

        Timer &operator = (const Timer &) = delete;

        /**
         * Schedule to be executed after the given timeout has passed. If this
         * timer was already scheduled, the previous scheduling is overridden.
         *
         * @param timeout   The timeout to wait until executed.
         * @param cb        The callback to be executed. The given callback
         *                  exists until executed or until canceled.
         *
         * @exception NullCallback if the given cb is nullptr.
         */
        void schedule(const boost::posix_time::time_duration    &timeout,
                      const Callback                            &cb);

        /**
         * Cancel this timer, if it was scheduled.
         */
        void cancel();

        /**
         * Is this timer scheduled?
         *
         * @return true, if this timer is scheduled.
         */
        bool isScheduled() const;

        void execute();

        virtual void dump(std::ostream &os) const;

    private:
        TimerQueue &tq;
        TimerPosition position;
        Callback cb;
    };
}

#endif /* !EVENTLOOP_TIMER_HEADER */
