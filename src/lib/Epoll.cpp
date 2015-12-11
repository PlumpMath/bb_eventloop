#include "private/Epoll.hpp"

#include <algorithm>

#include <EventLoop/SignalSet.hpp>
#include <EventLoop/DumpUtils.hpp>

#include "private/System.hpp"

using namespace EventLoop;

Epoll::Epoll(System &system):
    system(system),
    epfd(system, system.epollCreate(EPOLL_CLOEXEC)),
    addCount(0)
{
}

Epoll::~Epoll()
{
}

void Epoll::add(int                 fd,
                struct epoll_event  *event)
{
    system.epollCtl(epfd, EPOLL_CTL_ADD, fd, event);
    ++addCount;
}

void Epoll::modify(int                  fd,
                   struct epoll_event   *event)
{
    system.epollCtl(epfd, EPOLL_CTL_MOD, fd, event);
}

void Epoll::del(int fd)
{
    struct epoll_event event = { }; // See bugs in epoll_ctl(2)
    system.epollCtl(epfd, EPOLL_CTL_DEL, fd, &event);
    --addCount;
}

bool Epoll::wait(int timeout)
{
    eventArray.resize(addCount);
    const int ret(system.epollWait(epfd,
                                   &eventArray[0],
                                   static_cast<int>(eventArray.size()),
                                   timeout));
    eventArray.resize(static_cast<size_t>(std::max(0, ret)));
    return (ret == 0);
}

void Epoll::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_VALUE_OF(os, epfd);
    DUMP_VALUE_OF(os, addCount);
    DUMP_END(os);
}
