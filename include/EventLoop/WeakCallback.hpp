#ifndef EVENTLOOP_WEAKCALLBACK_HEADER
#define EVENTLOOP_WEAKCALLBACK_HEADER

#include <functional>
#include <memory>

#include <EventLoop/Callee.hpp>

namespace EventLoop
{
    /**
     * Weak callback that can be used together with Runner's addCallback().
     * Weak callback is associated with callee, whose function is expected to
     * be passed as the callback. If the callee is destroyed before this
     * callback, then this callback automatically becomes no-operation.
     *
     * @see Callee
     */
    class WeakCallback
    {
    public:
        WeakCallback() = default;

        explicit WeakCallback(const Callee                  &callee,
                              const std::function<void()>   &cb):
            wl(callee.getLife()),
            cb(cb)
        {
        }

        WeakCallback(const WeakCallback &) = default;

        WeakCallback &operator = (const WeakCallback &) = default;

        ~WeakCallback() = default;

        void operator () () { execute(); };

    private:
        void execute()
        {
            if (!wl.expired())
            {
                cb();
            }
        }

        std::weak_ptr<Callee::Life> wl;
        std::function<void()> cb;
    };
}

#endif /* !EVENTLOOP_WEAKCALLBACK_HEADER */
