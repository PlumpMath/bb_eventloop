#include "private/TimerQueue.hpp"

#include <ostream>
#include <algorithm>
#include <boost/date_time/posix_time/posix_time_io.hpp>

#include <EventLoop/Timer.hpp>
#include <EventLoop/DumpUtils.hpp>

#include "private/System.hpp"

using namespace EventLoop;

const boost::posix_time::time_duration TimerQueue::NO_TIMEOUT(boost::posix_time::milliseconds(-1));

TimerQueue::TimerQueue():
    TimerQueue(System::getSystem()) { }

TimerQueue::TimerQueue(System &system):
    system(system)
{
}

TimerQueue::~TimerQueue()
{
}

Timer *TimerQueue::popFirstTimerToExecute()
{
    Timer *timer(nullptr);
    const Queue::iterator i(queue.begin());
    if (i != queue.end())
    {
        timer = i->second;
        erase(i);
    }
    return timer;
}

boost::posix_time::time_duration TimerQueue::getTimeout() const
{
    const Queue::const_iterator i(queue.begin());
    if (i == queue.end())
    {
        return NO_TIMEOUT;
    }
    else
    {
        const boost::posix_time::time_duration timeout(i->first - system.getMonotonicClock());
        return timeout.is_negative() ? boost::posix_time::time_duration() : timeout;
    }
}

void TimerQueue::schedule(Timer                                     &timer,
                          const boost::posix_time::time_duration    &timeout)
{
    cancel(timer);
    const boost::posix_time::time_duration tte(system.getMonotonicClock() + timeout);
    timer.position = queue.insert(std::make_pair(tte, &timer));
}

void TimerQueue::cancel(const Timer &timer)
{
    if (timer.position != queue.end())
    {
        erase(timer.position);
    }
}

void TimerQueue::erase(TimerPosition i)
{
    i->second->position = queue.end();
    queue.erase(i);
}

bool TimerQueue::isEmpty() const
{
    return queue.empty();
}

void TimerQueue::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, system);
    DUMP_RESULT_OF(os, system.getMonotonicClock());
    DUMP_PAIR_CONTAINER(os, queue);
    DUMP_END(os);
}
