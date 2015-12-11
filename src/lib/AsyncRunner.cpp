#include <EventLoop/AsyncRunner.hpp>

#include <EventLoop/FDListener.hpp>
#include <EventLoop/FileDescriptor.hpp>
#include <EventLoop/DumpUtils.hpp>

#include "private/System.hpp"
#include "private/BaseEpollRunner.hpp"
#include "private/FDMonitor.hpp"

using namespace EventLoop;

class AsyncRunner::TimerFD: public FDListener
{
public:
    TimerFD() = delete;

    TimerFD(System          &system,
            FDListenerMap   &fdListenerMap);

    ~TimerFD();

    void disarm();

    void arm(const boost::posix_time::time_duration &timeout);

    void dump(std::ostream &os) const;

protected:
    void handleEventsCb(int             fd,
                        unsigned int    events);

private:
    System &system;
    FDListenerMap &fdListenerMap;
    FileDescriptor fd;
};

AsyncRunner::TimerFD::TimerFD(System        &system,
                              FDListenerMap &fdListenerMap):
    system(system),
    fdListenerMap(fdListenerMap),
    fd(system, system.timerFDCreate(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
{
    fdListenerMap.add(*this, fd, EVENT_IN);
}

AsyncRunner::TimerFD::~TimerFD()
{
    fdListenerMap.del(fd);
}

void AsyncRunner::TimerFD::handleEventsCb(int,
                                          unsigned int)
{
    /* Just reading all the data is enough */
    ssize_t ssize;
    do
    {
        char buf[32];
        ssize = system.read(fd, buf, sizeof(buf));
    }
    while (ssize > 0);
}

void AsyncRunner::TimerFD::disarm()
{
    static const struct itimerspec spec = { };
    system.timerFDSetTime(fd, 0, &spec, nullptr);
}

void AsyncRunner::TimerFD::arm(const boost::posix_time::time_duration &timeout)
{
    typedef boost::posix_time::time_duration::tick_type TickType;
    static const TickType NS(static_cast<TickType>(1000000000LL));
    const struct itimerspec spec = { { 0, 0 }, { timeout.total_seconds(), static_cast<long int>(timeout.total_nanoseconds() % NS) } };
    system.timerFDSetTime(fd, 0, &spec, nullptr);
}

void AsyncRunner::TimerFD::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, system);
    DUMP_ADDRESS_OF(os, fdListenerMap);
    DUMP_END(os);
}

AsyncRunner::AsyncRunner():
    AsyncRunner(System::getSystem()) { }

AsyncRunner::AsyncRunner(System &system):
    fdsDeletedWhileHandlingEvents(new FDMonitor()),
    baseEpollRunner(new BaseEpollRunner(system)),
    timerFD(new TimerFD(system, baseEpollRunner->fdListenerMap))
{
}

AsyncRunner::~AsyncRunner()
{
}

void AsyncRunner::step()
{
    waitAndHandleFDEvents();
    executeTimersAndUpdateTimer();
}

void AsyncRunner::waitAndHandleFDEvents()
{
    if (!baseEpollRunner->epoll.wait(0))
    {
        FDMonitor::Guard g(*fdsDeletedWhileHandlingEvents);
        /* Events in one or more fds, handle them */
        for (Epoll::event_iterator i = baseEpollRunner->epoll.events_begin();
             i != baseEpollRunner->epoll.events_end();
             ++i)
        {
            const int fd(i->data.fd);
            if (!fdsDeletedWhileHandlingEvents->has(fd))
            {
                FDListener &fl(baseEpollRunner->fdListenerMap.get(fd));
                fl.handleEvents(fd, i->events);
            }
        }
    }
}

void AsyncRunner::executeTimersAndUpdateTimer()
{
    while (true)
    {
        baseEpollRunner->cbq.execute();

        static const boost::posix_time::time_duration MIN_TIMEOUT(boost::posix_time::milliseconds(1));

        const boost::posix_time::time_duration timeout(baseEpollRunner->tq.getTimeout());
        if (timeout == TimerQueue::NO_TIMEOUT)
        {
            timerFD->disarm();
            break;
        }
        else if (timeout < MIN_TIMEOUT)
        {
            /* Already late or just in time, execute the first timer */
            baseEpollRunner->executeFirstTimer();
            continue;
        }
        else
        {
            timerFD->arm(timeout);
            break;
        }
    }
}

int AsyncRunner::getEventFD() const
{
    return baseEpollRunner->epoll.getFD();
}

void AsyncRunner::addFDListener(FDListener      &fl,
                                int             fd,
                                unsigned int    events)
{
    baseEpollRunner->fdListenerMap.add(fl, fd, events);
}

void AsyncRunner::modifyFDListener(int          fd,
                                   unsigned int events)
{
    baseEpollRunner->fdListenerMap.modify(fd, events);
}

void AsyncRunner::delFDListener(int fd)
{
    fdsDeletedWhileHandlingEvents->add(fd);
    baseEpollRunner->fdListenerMap.del(fd);
}

void AsyncRunner::addCallback(const std::function<void()> &cb)
{
    baseEpollRunner->cbq.add(cb);
}

TimerQueue &AsyncRunner::getTimerQueue()
{
    return baseEpollRunner->tq;
}

void AsyncRunner::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_VALUE_POINTED_BY(os, fdsDeletedWhileHandlingEvents);
    DUMP_VALUE_POINTED_BY(os, baseEpollRunner);
    DUMP_ADDRESS_OF(os, timerFD);
    DUMP_END(os);
}
