#include <EventLoop/FDListener.hpp>

#include <ostream>
#include <sys/epoll.h>

#include <EventLoop/DumpUtils.hpp>

using namespace EventLoop;

const unsigned int FDListener::EVENT_IN(EPOLLIN);
const unsigned int FDListener::EVENT_OUT(EPOLLOUT);
const unsigned int FDListener::EVENT_ERR(EPOLLERR);
const unsigned int FDListener::EVENT_HUP(EPOLLHUP);

FDListener::~FDListener()
{
}

void FDListener::handleEvents(int           fd,
                              unsigned int  events)
{
    handleEventsCb(fd, events);
}

void FDListener::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_END(os);
}
