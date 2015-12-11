#include "private/FDMonitor.hpp"

#include <EventLoop/DumpUtils.hpp>

using namespace EventLoop;

FDMonitor::Guard::Guard(FDMonitor &m):
    m(m)
{
    m.startMonitoring();
}

FDMonitor::Guard::~Guard()
{
    m.stopMonitoring();
}

FDMonitor::FDMonitor():
    monitoring(false)
{
}

FDMonitor::~FDMonitor()
{
}

void FDMonitor::startMonitoring()
{
    monitoring = true;
}

void FDMonitor::stopMonitoring()
{
    monitoring = false;
    set.clear();
}

void FDMonitor::add(int fd)
{
    if (monitoring)
    {
        set.insert(fd);
    }
}

bool FDMonitor::has(int fd) const
{
    return set.find(fd) != set.end();
}

void FDMonitor::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_VALUE_OF(os, monitoring);
    DUMP_CONTAINER(os, set);
    DUMP_END(os);
}
