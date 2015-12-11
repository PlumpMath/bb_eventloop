#ifndef EVENTLOOP_ABORT_HEADER
#define EVENTLOOP_ABORT_HEADER

namespace EventLoop
{
    void logAndAbort(const char *file, int line) noexcept __attribute__ ((__noreturn__));
}

#define EVENTLOOP_ABORT() EventLoop::logAndAbort(__FILE__, __LINE__)

#endif /* !EVENTLOOP_ABORT_HEADER */
