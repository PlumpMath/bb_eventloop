#ifndef EVENTLOOP_CALLBACKQUEUE_HEADER
#define EVENTLOOP_CALLBACKQUEUE_HEADER

#include <list>
#include <functional>
#include <boost/thread/mutex.hpp>

#include <EventLoop/Dumpable.hpp>

namespace EventLoop
{
    /**
     * Callback is simple but thread safe container for generic callbacks.
     * Added callbacks are kept and executed in FIFO order.
     */
    class CallbackQueue: public Dumpable
    {
    public:
        CallbackQueue();

        virtual ~CallbackQueue();

        CallbackQueue(const CallbackQueue &) = delete;

        CallbackQueue &operator = (const CallbackQueue &) = delete;

        typedef std::function<void()> Callback;

        void add(const Callback &cb);

        bool execute();

        typedef std::function<void()> WakeUp;

        void setWakeUp(WakeUp wakeUp);

        virtual void dump(std::ostream &os) const;

    private:
        typedef std::list<Callback> Queue;
        mutable boost::mutex queueLock;
        Queue queue;
        WakeUp wakeUp;

        Callback pop();
    };
}

#endif /* !EVENTLOOP_CALLBACKQUEUE_HEADER */
