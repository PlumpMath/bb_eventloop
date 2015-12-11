#include "private/Abort.hpp"

#include <cstdlib>
#include <syslog.h>

void EventLoop::logAndAbort(const char *file, int line) noexcept
{
    syslog(LOG_ERR, "abort called from file %s line %d\n", file, line);
    abort();
}
