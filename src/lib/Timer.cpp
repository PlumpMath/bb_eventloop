#include <EventLoop/Timer.hpp>

#include <ostream>

#include <EventLoop/Runner.hpp>
#include <EventLoop/DumpUtils.hpp>

#include "private/TimerQueue.hpp"

using namespace EventLoop;

Timer::Timer(Runner &runner):
    tq(runner.getTimerQueue()),
    position(tq.end())
{
}

Timer::~Timer()
{
    cancel();
}

void Timer::schedule(const boost::posix_time::time_duration &timeout,
                     const Callback                         &cb)
{
    if (!cb)
    {
        throw NullCallback();
    }
    this->cb = cb;
    tq.schedule(*this, timeout);
}

void Timer::cancel()
{
    tq.cancel(*this);
    cb = Callback();
}

bool Timer::isScheduled() const
{
    return (position != tq.end());
}

void Timer::execute()
{
    Callback copy(cb);
    cb = Timer::Callback();
    copy();
}

void Timer::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, tq);
    DUMP_VALUE_OF(os, position);
    DUMP_VALUE_OF(os, cb);
    DUMP_END(os);
}
