#ifndef EVENTLOOP_POSTPONEDELETION_HEADER
#define EVENTLOOP_POSTPONEDELETION_HEADER

#include <memory>

#include <EventLoop/Runner.hpp>

namespace EventLoop
{
    /**
     * Postpone deletion of the given shared pointer by keeping a reference
     * to it until the next 'runner loop'. This is handy way to make sure
     * a shared pointer is going to be deleted but not in the context of the
     * current function.
     */
    template<typename T>
    void postponeDeletion(Runner                &runner,
                          std::shared_ptr<T>    ptr)
    {
        runner.addCallback([ptr] () { static_cast<void>(ptr); });
    }
}

#endif /* !EVENTLOOP_POSTPONEDELETION_HEADER */
