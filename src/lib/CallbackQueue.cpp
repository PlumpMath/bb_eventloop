#include "private/CallbackQueue.hpp"

#include <ostream>

#include <EventLoop/Runner.hpp>
#include <EventLoop/DumpUtils.hpp>

using namespace EventLoop;

CallbackQueue::CallbackQueue()
{
}

CallbackQueue::~CallbackQueue()
{
}

void CallbackQueue::add(const Callback &cb)
{
    if (!cb)
    {
        throw Runner::NullCallback();
    }

    boost::unique_lock<boost::mutex> lock(queueLock);

    queue.push_back(cb);
    if ((queue.size() == 1) &&
        (wakeUp))
    {
        wakeUp();
    }
}

bool CallbackQueue::execute()
{
    bool ret(false);
    Callback cb(pop());
    while (cb)
    {
        ret = true;
        cb();
        cb = pop();
    }
    return ret;
}

void CallbackQueue::setWakeUp(WakeUp wakeUp)
{
    boost::unique_lock<boost::mutex> lock(queueLock);

    this->wakeUp = wakeUp;
}

CallbackQueue::Callback CallbackQueue::pop()
{
    boost::unique_lock<boost::mutex> lock(queueLock);

    Callback cb;
    if (!queue.empty())
    {
        cb = queue.front();
        queue.pop_front();
    }
    return cb;
}

void CallbackQueue::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    {
        boost::unique_lock<boost::mutex> lock(queueLock);
        DUMP_CONTAINER(os, queue);
        DUMP_VALUE_OF(os, wakeUp);
    }
    DUMP_END(os);
}
