#include "private/BaseEpollRunner.hpp"

#include <EventLoop/Timer.hpp>
#include <EventLoop/DumpUtils.hpp>

using namespace EventLoop;

BaseEpollRunner::BaseEpollRunner(System &system):
    tq(system),
    fdListenerMap(std::bind(&BaseEpollRunner::addFDListener, this, std::placeholders::_2, std::placeholders::_3),
                  std::bind(&BaseEpollRunner::modifyFDListener, this, std::placeholders::_2, std::placeholders::_3),
                  std::bind(&BaseEpollRunner::delFDListener, this, std::placeholders::_2)),
    epoll(system)
{
}

BaseEpollRunner::~BaseEpollRunner()
{
}

void BaseEpollRunner::executeFirstTimer()
{
    Timer *timer(tq.popFirstTimerToExecute());
    if (timer != nullptr)
    {
        timer->execute();
    }
}

void BaseEpollRunner::addFDListener(int fd, unsigned int events)
{
    struct epoll_event event = { };
    event.events = events;
    event.data.fd = fd;
    epoll.add(fd, &event);
}

void BaseEpollRunner::modifyFDListener(int fd, unsigned int events)
{
    struct epoll_event event = { };
    event.events = events;
    event.data.fd = fd;
    epoll.modify(fd, &event);
}

void BaseEpollRunner::delFDListener(int fd)
{
    epoll.del(fd);
}

void BaseEpollRunner::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_VALUE_OF(os, cbq);
    DUMP_VALUE_OF(os, tq);
    DUMP_VALUE_OF(os, fdListenerMap);
    DUMP_VALUE_OF(os, epoll);
    DUMP_END(os);
}
